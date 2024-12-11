#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <ctype.h>
#include <unistd.h>
#include <sys/stat.h>
#include <pwd.h>

void get_memory_info(long *total_mem, long *used_mem, long *mem_buffers, long *mem_cached, long *mem_free, long *mem_available, long *kreclaimable, long *total_swap, long *free_swap) {
    FILE *fp = fopen("/proc/meminfo", "r");
    if (fp == NULL) {
        perror("Failed to read /proc/meminfo");
        *total_mem = *used_mem = *mem_buffers = *mem_cached = *mem_free = *mem_available = *kreclaimable = *total_swap = *free_swap = -1;
        return;
    }

    char label[256];
    long value;

    *total_mem = *mem_free = *mem_buffers = *mem_cached = *mem_available = *kreclaimable = *total_swap = *free_swap = 0;

    while (fscanf(fp, "%31s %ld kB", label, &value) != EOF) {
        if (strcmp(label, "MemTotal:") == 0) {
            *total_mem = value;
        } else if (strcmp(label, "MemFree:") == 0) {
            *mem_free = value;
        } else if (strcmp(label, "Buffers:") == 0) {
            *mem_buffers = value;
        } else if (strcmp(label, "Cached:") == 0) {
            *mem_cached = value;
        } else if (strcmp(label, "MemAvailable:") == 0) {
            *mem_available = value;
        } else if (strcmp(label, "KReclaimable:") == 0) {
            *kreclaimable = value;
        } else if (strcmp(label, "SwapTotal:") == 0) {
            *total_swap = value;
        } else if (strcmp(label, "SwapFree:") == 0) {
            *free_swap = value;
        }

        // Exit loop early if all required values are retrieved
        if (*total_mem && *mem_free && *mem_buffers && *mem_cached && *mem_available && *kreclaimable && total_swap && free_swap) {
            break;
        }
    }
    fclose(fp);

    // Used memory = Total memory - Free memory
    *used_mem = *total_mem - *mem_available - *mem_free;
}