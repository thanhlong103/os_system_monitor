#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

// Function to get the number of CPU cores
int get_core_count() {
    return sysconf(_SC_NPROCESSORS_ONLN);
}

// Function to get the number of threads per core
  int get_thread_count() {
    int threads = 0;
    char path[64];
    FILE *fp;
    
    // Counting thread directories under each core in /sys/devices/system/cpu
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

// Function to calculate CPU usage percentage
double get_cpu_usage() {
    FILE *fp;
    long idle1, idle2, total1, total2;
    long user, nice, system, idle, iowait, irq, softirq, steal;
    
    // First read of CPU stats
    fp = fopen("/proc/stat", "r");
    if (fp == NULL) return -1;
    fscanf(fp, "cpu %ld %ld %ld %ld %ld %ld %ld %ld", &user, &nice, &system, &idle, &iowait, &irq, &softirq, &steal);
    fclose(fp);
    idle1 = idle + iowait;
    total1 = user + nice + system + idle + iowait + irq + softirq + steal;

    sleep(1); // Wait 1 second

    // Second read of CPU stats
    fp = fopen("/proc/stat", "r");
    if (fp == NULL) return -1;
    fscanf(fp, "cpu %ld %ld %ld %ld %ld %ld %ld %ld", &user, &nice, &system, &idle, &iowait, &irq, &softirq, &steal);
    fclose(fp);
    idle2 = idle + iowait;
    total2 = user + nice + system + idle + iowait + irq + softirq + steal;

    // Calculate the percentage of CPU usage
    double usage = (1.0 - ((double)(idle2 - idle1) / (total2 - total1))) * 100.0;
    return usage;
}

// Function to get total and available memory
void get_memory_info(long *total_mem, long *used_mem) {
    FILE *fp = fopen("/proc/meminfo", "r");
    if (fp == NULL) {
        *total_mem = *used_mem = -1;
        return;
    }

    char label[32];
    long mem_total = 0, mem_free = 0, mem_available = 0;
    
    // Read the file line by line to extract memory information
    while (fscanf(fp, "%31s %ld kB", label, &mem_total) != EOF) {
        if (strcmp(label, "MemTotal:") == 0) {
            *total_mem = mem_total;
        } else if (strcmp(label, "MemAvailable:") == 0) {
            mem_available = mem_total;
            break;
        }
    }
    fclose(fp);

    // Calculate used memory
    *used_mem = *total_mem - mem_available;
}

int main() {
    int cores = get_core_count();
    int threads = get_thread_count();
    double cpu_usage = get_cpu_usage();

    long total_mem = 0, used_mem = 0;
    get_memory_info(&total_mem, &used_mem);
    double mem_usage = (double)used_mem / total_mem * 100.0;

    printf("Number of CPU cores: %d\n", cores);
    printf("Number of CPU threads: %d\n", threads);
    printf("CPU usage: %.2f%%\n", cpu_usage);
    printf("Total memory: %.2f MB\n", total_mem / 1024.0);
    printf("Used memory: %.2f MB\n", used_mem / 1024.0);
    printf("Memory usage: %.2f%%\n", mem_usage);

    return 0;
}
