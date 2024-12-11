#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include "cpu.h"
#include <pwd.h>
#include <ctype.h>
#include "process.h"

#define MAX_PROCESSES 4096
#define INITIAL_PROCESS_CAPACITY 1024

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
    } else if (prev_data_count < MAX_PROCESSES) {
        prev_data[prev_data_count].pid = pid;
        prev_data[prev_data_count].prev_total_time = total_time;
        prev_data[prev_data_count].prev_process_uptime = process_uptime;
        prev_data_count++;
    }
}

long get_uptime() {
    FILE *fp = fopen("/proc/uptime", "r");
    if (!fp) {
        perror("Failed to read /proc/uptime");
        return -1;
    }
    long uptime;
    fscanf(fp, "%ld", &uptime);
    fclose(fp);
    return uptime;
}

long get_total_memory() {
    FILE *fp = fopen("/proc/meminfo", "r");
    if (!fp) {
        perror("Failed to read /proc/meminfo");
        return -1;
    }
    long total_memory = 0;
    char line[256];
    while (fgets(line, sizeof(line), fp)) {
        if (strncmp(line, "MemTotal:", 9) == 0) {
            sscanf(line, "MemTotal: %ld", &total_memory);
            break;
        }
    }
    fclose(fp);
    return total_memory;
}

void read_process_info(const char *entry_name, long system_uptime, long clock_ticks, long total_memory, ProcessInfo *process) {
    char path[512];
    snprintf(path, sizeof(path), "/proc/%s/stat", entry_name);
    FILE *fp = fopen(path, "r");
    if (!fp) return;

    int pid;
    char comm[256], state;
    long utime, stime, starttime;
    int priority, nice; 

    if (fscanf(fp, "%d (%255[^)]) %c", &pid, comm, &state) != 3) {
        fclose(fp);
        return;
    }
    for (int i = 0; i < 10; i++) fscanf(fp, "%*s");
    fscanf(fp, "%ld %ld %*s %*s %d %d %*s %ld", &utime, &stime, &priority, &nice, &starttime);
    fclose(fp);
    
    long total_time = utime + stime;
    double process_uptime = system_uptime - (starttime / clock_ticks);
    double cpu_usage = 0.0;

    PrevProcessData *prev = find_prev_data(pid);
    if (prev && process_uptime > prev->prev_process_uptime) {
        cpu_usage = (double)(total_time - prev->prev_total_time) * 100.0 /
                    (clock_ticks * (process_uptime - prev->prev_process_uptime));
    }
    update_prev_data(pid, total_time, process_uptime);

    long mem_usage, virtual_mem, rss_file, rss_sh = 0;
    snprintf(path, sizeof(path), "/proc/%s/status", entry_name);
    fp = fopen(path, "r");
    if (fp) {
        char line[256];
        while (fgets(line, sizeof(line), fp)) {
            if (strncmp(line, "VmRSS:", 6) == 0) {
                sscanf(line, "VmRSS: %ld", &mem_usage);
            }
            if (strncmp(line, "VmSize:", 7) == 0) {
                sscanf(line, "VmSize: %ld", &virtual_mem);
            }
            if (strncmp(line, "RssFile:", 8) == 0) {
                sscanf(line, "RssFile: %ld", &rss_file);
            }
            if (strncmp(line, "RssShmem:", 9) == 0) {
                sscanf(line, "RssShmem: %ld", &rss_sh);
            }
        }
        fclose(fp);
    }

    long shared_mem = rss_file + rss_sh;
    double mem_usage_percentage = (total_memory > 0) ? (mem_usage / (double)total_memory) * 100.0 : 0.0;

    snprintf(path, sizeof(path), "/proc/%s", entry_name);
    struct stat statbuf;
    if (stat(path, &statbuf) == 0) {
        struct passwd *pwd = getpwuid(statbuf.st_uid);
        strncpy(process->user, pwd ? pwd->pw_name : "unknown", sizeof(process->user) - 1);
    } else {
        strcpy(process->user, "unknown");
    }

    process->pid = pid;
    process->status[0] = state;
    process->status[1] = '\0';
    process->virtual_mem = virtual_mem;
    process->res_mem = mem_usage;
    process->shared_mem = shared_mem;
    strncpy(process->name, comm, sizeof(process->name) - 1);
    process->cpu_usage = cpu_usage;
    process->mem_usage = mem_usage_percentage;
    process->nice = nice;

    if (priority == -100) {
        strcpy(process->priority, "rt");
    } else {
        snprintf(process->priority, sizeof(process->priority), "%d", priority);
    }
}

int compare_cpu_usage(const void *a, const void *b) {
    double usage_a = ((ProcessInfo *)a)->cpu_usage;
    double usage_b = ((ProcessInfo *)b)->cpu_usage;
    return (usage_b > usage_a) - (usage_b < usage_a);
}

void list_processes() {
    DIR *dir = opendir("/proc");
    if (!dir) {
        perror("Failed to open /proc directory");
        return;
    }

    long system_uptime = get_uptime();
    long total_memory = get_total_memory();
    long clock_ticks = sysconf(_SC_CLK_TCK);
    if (system_uptime == -1 || total_memory == -1) {
        closedir(dir);
        return;
    }

    int process_count = 0;
    ProcessInfo *processes = malloc(INITIAL_PROCESS_CAPACITY * sizeof(ProcessInfo));
    if (!processes) {
        perror("Failed to allocate memory for process list");
        closedir(dir);
        return;
    }
    int capacity = INITIAL_PROCESS_CAPACITY;

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (!isdigit(entry->d_name[0])) continue;

        if (process_count >= capacity) {
            capacity *= 2;
            processes = realloc(processes, capacity * sizeof(ProcessInfo));
            if (!processes) {
                perror("Failed to reallocate memory");
                closedir(dir);
                return;
            }
        }

        read_process_info(entry->d_name, system_uptime, clock_ticks, total_memory, &processes[process_count]);
        process_count++;
    }
    closedir(dir);

    qsort(processes, process_count, sizeof(ProcessInfo), compare_cpu_usage);

    printf("\033[1;36m%-6s %-12s %-10s %-5s %-15s %-15s %-15s %-8s %-15s %-15s %-45s\033[0m\n", 
        "PID", "USER", "PRIORITY", "NICE", "VIRTUAL_MEM", "RES_MEM", "SH_MEM", "STATUS", "CPU_USAGE(%)", "MEM_USAGE(%)", "NAME");
    printf("--------------------------------------------------------------------------------------------------------------------------------------------------\n");

    for (int i = 0; i < process_count && i < 10; i++) {
        printf("\033[1;32m%-6d \033[1;33m%-12s \033[1;37m%-10s\033[0m \033[1;32m%-5d \033[1;35m%-15ld \033[1;35m%-15ld \033[1;35m%-15ld \033[1;34m%-8s \033[1;34m%-15.2f \033[1;31m%-15.2f \033[1;35m%-45s\n",
            processes[i].pid,
            processes[i].user,
            processes[i].priority,
            processes[i].nice,
            processes[i].virtual_mem,
            processes[i].res_mem,
            processes[i].shared_mem,
            processes[i].status,
            processes[i].cpu_usage,
            processes[i].mem_usage,
            processes[i].name);
    }

    free(processes);
}