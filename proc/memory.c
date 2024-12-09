#include <stdio.h>      // For printf, perror, fopen, fgets, fscanf, etc.
#include <stdlib.h>     // For malloc, realloc, free, qsort, etc.
#include <string.h>     // For strncpy, strcmp, strcspn, etc.
#include <dirent.h>     // For opendir, readdir, closedir, struct dirent
#include <ctype.h>      // For isdigit
#include <unistd.h>     // For sysconf
#include <sys/stat.h>   // For stat, struct stat
#include <pwd.h> 
#include "memory.h"

void get_memory_info(long *total_mem, long *used_mem, long *mem_buffers, long *mem_cached, long *mem_free, long *mem_available, long *kreclaimable) {
    FILE *fp = fopen("/proc/meminfo", "r");
    if (fp == NULL) {
        perror("Failed to read /proc/meminfo");
        *total_mem = *used_mem = *mem_buffers = *mem_cached = *mem_free = *mem_available = *kreclaimable = -1;
        return;
    }

    char label[256];
    long value;

    *total_mem = *mem_free = *mem_buffers = *mem_cached = *mem_available = *kreclaimable = 0;

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
        }

        // Exit loop early if all required values are retrieved
        if (*total_mem && *mem_free && *mem_buffers && *mem_cached && *mem_available && *kreclaimable) {
            break;
        }
    }
    fclose(fp);

    // Used memory = Total memory - Free memory
    *used_mem = *total_mem - *mem_available;
}