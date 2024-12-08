#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/types.h>
#include <pwd.h>
#include <unistd.h>
#include <sys/stat.h>
#include <ctype.h>
#include <time.h>
#include <signal.h>
#include <sys/time.h>

// Function prototypes
int get_core_count();
int get_thread_count();
double get_cpu_usage();
void get_memory_info(long *total_mem, long *used_mem, long *mem_buffers, long *mem_cached, long *mem_free);
void list_processes();

// Structure to hold process information
typedef struct {
    int pid;
    char user[256];
    char name[256];
    double cpu_usage;
    double mem_usage;
} ProcessInfo;

// Function to compare CPU usage for sorting
int compare_cpu_usage(const void *a, const void *b) {
    double usage_a = ((ProcessInfo *)a)->cpu_usage;
    double usage_b = ((ProcessInfo *)b)->cpu_usage;
    return (usage_b - usage_a) > 0 ? 1 : -1;
}

// Function to get the number of CPU cores
int get_core_count() {
    return sysconf(_SC_NPROCESSORS_ONLN);
}

// Function to get the number of threads per core
int get_thread_count() {
    int threads = 0;
    char path[64];
    FILE *fp;

    for (int i = 0; i < get_core_count(); i++) {
        snprintf(path, sizeof(path), "/sys/devices/system/cpu/cpu%d/topology/thread_siblings_list", i);
        fp = fopen(path, "r");
        if (fp) {
            threads++;
            fclose(fp);
        }
    }
    return threads;
}

// Function to calculate CPU usage with breakdown of percentages
double get_cpu_usage(double *user_pct, double *system_pct, double *idle_pct, double *iowait_pct, double *irq_pct, double *softirq_pct, double *steal_pct) {
    static long prev_total = 0;
    static long prev_user = 0;
    static long prev_system = 0;
    static long prev_idle = 0;
    static long prev_iowait = 0;
    static long prev_irq = 0;
    static long prev_softirq = 0;
    static long prev_steal = 0;
    long user, nice, system, idle, iowait, irq, softirq, steal;
    long total, totald, userd, systemd, idled, iowaitd, irqd, softirqd, steald;
    double cpu_usage;

    FILE *fp = fopen("/proc/stat", "r");
    if (fp == NULL) {
        perror("Failed to read /proc/stat");
        return -1.0;
    }

    // Read CPU time values from /proc/stat
    if (fscanf(fp, "cpu %ld %ld %ld %ld %ld %ld %ld %ld", 
               &user, &nice, &system, &idle, &iowait, &irq, &softirq, &steal) != 8) {
        fclose(fp);
        return -1.0;
    }
    fclose(fp);

    // Calculate current total and idle times
    long current_user = user + nice;
    long current_total = user + nice + system + iowait + irq + softirq + steal + idle;

    // If previous values are zero (first call), initialize them
    if (prev_total == 0 && prev_idle == 0) {
        prev_total = current_total;
        prev_user = current_user;
        prev_idle = idle;
        prev_system = system;
        prev_iowait = iowait;
        prev_irq = irq;
        prev_softirq = softirq;
        prev_steal = steal;
        return 0.0; // No usage to calculate on the first call
    }

    // Calculate deltas
    totald = current_total - prev_total;
    userd = current_user - prev_user;
    systemd = system - prev_system;
    idled = idle - prev_idle;
    iowaitd = iowait - prev_iowait;
    irqd = irq - prev_irq;
    softirqd = softirq - prev_softirq;
    steald = steal - prev_steal;

    // Update previous values for the next calculation
    prev_total = current_total;
    prev_user = current_user;
    prev_idle = idle;
    prev_system = system;
    prev_iowait = iowait;
    prev_irq = irq;
    prev_softirq = softirq;
    prev_steal = steal;

    // Calculate overall CPU usage percentage
    cpu_usage = (totald - idled) / (double)totald * 100.0;

    // Calculate breakdown percentages
    *user_pct = userd / (double)totald * 100.0;
    *system_pct = systemd / (double)totald * 100.0;
    *idle_pct = idled / (double)totald * 100.0;
    *iowait_pct = iowaitd / (double)totald * 100.0;
    *irq_pct = irqd / (double)totald * 100.0;
    *softirq_pct = softirqd / (double)totald * 100.0;
    *steal_pct = steald / (double)totald * 100.0;

    return cpu_usage;
}


void get_memory_info(long *total_mem, long *used_mem, long *mem_buffers, long *mem_cached, long *mem_free) {
    FILE *fp = fopen("/proc/meminfo", "r");
    if (fp == NULL) {
        perror("Failed to read /proc/meminfo");
        *total_mem = *used_mem = *mem_buffers = *mem_cached = *mem_free = -1;
        return;
    }

    char label[256];
    long value;

    *total_mem = *mem_free = *mem_buffers = *mem_cached = 0;

    while (fscanf(fp, "%31s %ld kB", label, &value) != EOF) {
        if (strcmp(label, "MemTotal:") == 0) {
            *total_mem = value;
        } else if (strcmp(label, "MemFree:") == 0) {
            *mem_free = value;
        } else if (strcmp(label, "Buffers:") == 0) {
            *mem_buffers = value;
        } else if (strcmp(label, "Cached:") == 0) {
            *mem_cached = value;
        }

        // Exit loop early if all required values are retrieved
        if (*total_mem && *mem_free && *mem_buffers && *mem_cached) {
            break;
        }
    }
    fclose(fp);

    // Used memory = Total memory - Free memory
    *used_mem = *total_mem - (*mem_free + *mem_buffers + *mem_cached);
}

// Function to list processes and display them in a formatted table
void list_processes() {
    DIR *dir;
    struct dirent *entry;
    char path[512];
    char process_name[256];
    struct stat statbuf;
    FILE *fp;
    long system_uptime;
    long clock_ticks = sysconf(_SC_CLK_TCK);
    long total_memory = 0;
    ProcessInfo *processes = NULL;
    int process_count = 0;
    int max_processes = 1024;
    processes = malloc(max_processes * sizeof(ProcessInfo));

    if (processes == NULL) {
        perror("Failed to allocate memory for process list");
        return;
    }

    fp = fopen("/proc/meminfo", "r");
    if (fp) {
        char line[256];
        while (fgets(line, sizeof(line), fp)) {
            if (strncmp(line, "MemTotal:", 9) == 0) {
                sscanf(line, "MemTotal: %ld", &total_memory);
                break;
            }
        }
        fclose(fp);
    } else {
        perror("Failed to read /proc/meminfo");
        free(processes);
        return;
    }

    fp = fopen("/proc/uptime", "r");
    if (fp == NULL) {
        perror("Failed to read /proc/uptime");
        free(processes);
        return;
    }
    fscanf(fp, "%ld", &system_uptime);
    fclose(fp);

    if ((dir = opendir("/proc")) == NULL) {
        perror("opendir");
        free(processes);
        return;
    }

    while ((entry = readdir(dir)) != NULL) {
        if (!isdigit(entry->d_name[0])) {
            continue;
        }

        snprintf(path, sizeof(path), "/proc/%s/stat", entry->d_name);
        fp = fopen(path, "r");
        if (fp == NULL) {
            continue;
        }

        int pid;
        char comm[256], state;
        long utime, stime, cutime, cstime, starttime;
        fscanf(fp, "%d %s %c", &pid, comm, &state);

        for (int i = 0; i < 10; i++) {
            fscanf(fp, "%*s");
        }

        fscanf(fp, "%ld %ld %ld %ld %*s %*s %*s %*s %ld", 
               &utime, &stime, &cutime, &cstime, &starttime);
        fclose(fp);

        long total_time = utime + stime;
        double process_uptime = system_uptime - (starttime / clock_ticks);
        double cpu_usage = (process_uptime > 0) ? (total_time / (double)clock_ticks / process_uptime) * 100.0 : 0.0;

        snprintf(path, sizeof(path), "/proc/%s/comm", entry->d_name);
        fp = fopen(path, "r");
        if (fp) {
            if (fgets(process_name, sizeof(process_name), fp)) {
                process_name[strcspn(process_name, "\n")] = '\0';
            } else {
                strcpy(process_name, "unknown");
            }
            fclose(fp);
        } else {
            strcpy(process_name, "unknown");
        }

        long mem_usage = 0;
        snprintf(path, sizeof(path), "/proc/%s/status", entry->d_name);
        fp = fopen(path, "r");
        if (fp) {
            char line[256];
            while (fgets(line, sizeof(line), fp)) {
                if (strncmp(line, "VmRSS:", 6) == 0) {
                    sscanf(line, "VmRSS: %ld", &mem_usage);
                    break;
                }
            }
            fclose(fp);
        }

        double mem_usage_percentage = (total_memory > 0) ? (mem_usage / (double)total_memory) * 100.0 : 0.0;

        if (process_count >= max_processes) {
            max_processes *= 2;
            processes = realloc(processes, max_processes * sizeof(ProcessInfo));
            if (processes == NULL) {
                perror("Failed to reallocate memory for process list");
                return;
            }
        }

        processes[process_count].pid = pid;
        strncpy(processes[process_count].name, process_name, sizeof(processes[process_count].name) - 1);

        snprintf(path, sizeof(path), "/proc/%s", entry->d_name);
        if (stat(path, &statbuf) == 0) {
            struct passwd *pwd = getpwuid(statbuf.st_uid);
            strncpy(processes[process_count].user, (pwd != NULL) ? pwd->pw_name : "unknown",
                    sizeof(processes[process_count].user) - 1);
        } else {
            strcpy(processes[process_count].user, "unknown");
        }
        processes[process_count].cpu_usage = cpu_usage;
        processes[process_count].mem_usage = mem_usage_percentage; // Memory usage in percentage
        process_count++;
    }
    closedir(dir);

    qsort(processes, process_count, sizeof(ProcessInfo), compare_cpu_usage);

    printf("\n\033[1;36m%-6s %-12s %-20s %-15s %-15s\033[0m\n", "PID", "USER", "NAME", "CPU_USAGE(%)", "MEM_USAGE(%)");
    printf("-----------------------------------------------------------------------------------\n");

    for (int i = 0; i < process_count && i < 10; i++) {
        printf("\033[1;32m%-6d \033[1;33m%-12s \033[1;35m%-20s \033[1;34m%-15.2f \033[1;31m%-15.2f\033[0m\n",
               processes[i].pid,
               processes[i].user,
               processes[i].name,
               processes[i].cpu_usage,
               processes[i].mem_usage);
    }

    free(processes);
}

void get_task_info(int *running, int *sleeping, int *stopped, int *zombie, int *total) {
    DIR *dir = opendir("/proc");
    if (!dir) {
        perror("Failed to open /proc");
        return;
    }

    struct dirent *entry;
    *running = *sleeping = *stopped = *zombie = *total = 0;

    while ((entry = readdir(dir)) != NULL) {
        if (isdigit(entry->d_name[0])) { // Only process directories that are numeric (representing PIDs)
            char path[256];
            // Ensure the path buffer size is not exceeded
            if (snprintf(path, sizeof(path), "/proc/%s/stat", entry->d_name) >= sizeof(path)) {
                fprintf(stderr, "Warning: PID name too long: %s\n", entry->d_name);
                continue;
            }

            FILE *fp = fopen(path, "r");
            if (fp) {
                char status;
                fscanf(fp, "%*d %*s %c", &status);
                fclose(fp);

                // Increment the total count of processes
                (*total)++;

                // Count tasks based on their status
                switch (status) {
                    case 'R': // Running
                        (*running)++;
                        break;
                    case 'S': // Sleeping
                    case 'D': // Uninterruptible sleep (usually I/O)
                    case 'W': // Sleeping (low-priority)
                    case 'X': // Sleeping (in kernel)
                    case 'x': // Sleeping (in kernel)
                    case 'K': // Sleeping (in kernel)
                    case 'P': // Sleeping (in kernel)
                    case 'I': // Sleeping (in kernel)
                    case 'C': // Sleeping (in kernel)
                        (*sleeping)++;
                        break;
                    case 'T': // Stopped (traced or stopped by a signal)
                    case 't': // Stopped (traced or stopped by a signal)
                        (*stopped)++;
                        break;
                    case 'Z': // Zombie
                        (*zombie)++;
                        break;
                    default:
                        // Handle unexpected cases or unknown states
                        fprintf(stderr, "Unknown process status: %c\n", status);
                        break;
                }
            }
        }
    }

    closedir(dir);
}

void the_main() {

    double user_pct, system_pct, idle_pct, iowait_pct, irq_pct, softirq_pct, steal_pct;
    double cpu_usage = get_cpu_usage(&user_pct, &system_pct, &idle_pct, &iowait_pct, &irq_pct, &softirq_pct, &steal_pct);
    printf("\033[H\033[2J\033[3J");  // Clear the screen completely

    // Title Header
    printf("\033[1;44;97m%-50s\033[0m\n", " SYSTEM MONITOR ");
    printf("\033[1;37m%-50s\033[0m\n", "Real-time System Overview");

    int running, sleeping, stopped, zombie, total;

    get_task_info(&running, &sleeping, &stopped, &zombie, &total);

    // Print task statistics
    printf("\n\033[1;36mTask Statistics:\033[0m\n");
    printf("Total Processes: %d\n", total);
    printf("Running: %d\n", running);
    printf("Sleeping: %d\n", sleeping);
    printf("Stopped: %d\n", stopped);
    printf("Zombie: %d\n", zombie);
    printf("==============================================\n\n");

    // CPU Information Section
    printf("\033[1;34m%-50s\033[0m\n", "CPU INFORMATION:");
    printf("\033[37m%-25s : \033[1;32m%-15.1f%%\033[0m\n", "Total CPU Usage", cpu_usage);
    printf("\033[37m%-25s : \033[1;33mus=%.1f%% sy=%.1f%% id=%.1f%% io=%.1f%% hi=%.1f%% si=%.1f%% st=%.1f%%\033[0m\n", 
           "Usage Breakdown", user_pct, system_pct, idle_pct, iowait_pct, irq_pct, softirq_pct, steal_pct);
    printf("\n");

    // Memory Information Section
    long total_mem, used_mem, mem_free, mem_cached, mem_buffers;
    get_memory_info(&total_mem, &used_mem, &mem_buffers, &mem_cached, &mem_free);

    printf("\033[1;34m%-50s\033[0m\n", "MEMORY INFORMATION:");
    printf("\033[37m%-25s : \033[1;32m%-15.2f MB\033[0m\n", "Total Memory", total_mem / 1024.0);
    printf("\033[37m%-25s : \033[1;31m%-15.2f MB\033[0m\n", "Used Memory", used_mem / 1024.0);
    printf("\033[37m%-25s : \033[1;31m%-15.2f MB\033[0m\n", "Free Memory", mem_free / 1024.0);
    printf("\033[37m%-25s : \033[1;31m%-15.2f MB\033[0m\n", "Buffer Memory", mem_buffers / 1024.0);
    printf("\033[37m%-25s : \033[1;31m%-15.2f MB\033[0m\n", "Cache Memory", mem_cached / 1024.0);
    printf("\033[37m%-25s : \033[1;36m%-15.2f%%\033[0m\n", "Memory Usage", (double)used_mem / total_mem * 100.0);
    printf("\n");

    // Top Processes Section
    printf("\033[1;34m%-50s\033[0m\n", "PROCESSES BY CPU USAGE:");
    // printf("\033[37m%-5s %-20s %-10s %-10s %-10s\033[0m\n", "PID", "COMMAND", "CPU%", "MEM%", "STATE");
    printf("==============================================\n");
    list_processes();

    // Footer
    printf("\n\033[1;44;97m%-50s\033[0m\n", " Data Refreshed ");
    printf("\033[1;30m%-50s\033[0m\n", "Press Ctrl+C to exit.");

    fflush(stdout);  // Ensure output is written immediately
}


int main() {
    struct sigaction sa;
    struct itimerval timer;

    // Install the timer handler
    sa.sa_handler = the_main;
    sa.sa_flags = 0;  // or SA_RESTART to restart interrupted system calls
    sigaction(SIGALRM, &sa, NULL);

    // Configure the timer to expire after 1 second
    timer.it_value.tv_sec = 1;       // First timer expiry after 1 second
    timer.it_value.tv_usec = 0;

    // Configure the timer to restart every 1 second
    timer.it_interval.tv_sec = 1;   // Periodic interval of 1 second
    timer.it_interval.tv_usec = 0;

    // Start the timer
    setitimer(ITIMER_REAL, &timer, NULL);

    while (1) {
        sleep(1);  // Update every 1 second
    }
    return 0;
}
