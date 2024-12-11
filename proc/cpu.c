#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include "cpu.h"
#include <pwd.h>

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

#define MAX_PROCESSES 4096

PrevProcessData prev_data[MAX_PROCESSES];
int prev_data_count = 0;

PrevProcessData *find_prev_data(int pid) {
    for (int i = 0; i < prev_data_count; i++) {
        if (prev_data[i].pid == pid) {
            return &prev_data[i];
        }
    }
    return NULL;
}

void update_prev_data(int pid, long total_time, double process_uptime) {
    PrevProcessData *data = find_prev_data(pid);
    if (data) {
        data->prev_total_time = total_time;
        data->prev_process_uptime = process_uptime;
    } else {
        if (prev_data_count < MAX_PROCESSES) {
            prev_data[prev_data_count].pid = pid;
            prev_data[prev_data_count].prev_total_time = total_time;
            prev_data[prev_data_count].prev_process_uptime = process_uptime;
            prev_data_count++;
        }
    }
}

int compare_cpu_usage(const void *a, const void *b) {
    double usage_a = ((ProcessInfo *)a)->cpu_usage;
    double usage_b = ((ProcessInfo *)b)->cpu_usage;
    return (usage_b - usage_a) > 0 ? 1 : -1;
}

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
    // ProcessInfo *processes = NULL;
    int process_count = 0;
    int max_processes = 1024;
    ProcessInfo *processes = malloc(MAX_PROCESSES * sizeof(ProcessInfo));

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

        int pid;
        char comm[256], state;
        long utime, stime, starttime;

        snprintf(path, sizeof(path), "/proc/%s/stat", entry->d_name);
        fp = fopen(path, "r");
        if (fp) {
            // Read process data, including PID and command name (comm)
            fscanf(fp, "%d (%255[^)]) %c", &pid, comm, &state); // Extract name without parentheses
            for (int i = 0; i < 10; i++) fscanf(fp, "%*s");
            fscanf(fp, "%ld %ld %*s %*s %*s %*s %*s %ld", &utime, &stime, &starttime);
            fclose(fp);
        } else {
            continue; // Skip this process if the stat file can't be read
}
        long total_time = utime + stime;
        double process_uptime = system_uptime - (starttime / clock_ticks);

        double cpu_usage = 0.0;
        PrevProcessData *prev = find_prev_data(pid);
        if (prev && process_uptime > prev->prev_process_uptime) {
            cpu_usage = (double)(total_time - prev->prev_total_time) * 100 /
                        (clock_ticks * (process_uptime - prev->prev_process_uptime - 0.001));
        }

        update_prev_data(pid, total_time, process_uptime);

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

        if (process_count >= MAX_PROCESSES) {
            max_processes *= 2;
            processes = realloc(processes, MAX_PROCESSES * sizeof(ProcessInfo));
            if (processes == NULL) {
                perror("Failed to reallocate memory for process list");
                return;
            }
        }

        processes[process_count].pid = pid; 
        strncpy(processes[process_count].name, comm, sizeof(processes[process_count].name) - 1);
        processes[process_count].name[sizeof(processes[process_count].name) - 1] = '\0'; // Ensure null-termination


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