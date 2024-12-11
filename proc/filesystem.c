#include <stdio.h>
#include <stdlib.h>
#include <sys/statvfs.h>
#include <string.h>
#include "ui.h"

#define MAX_FS 1000  // Maximum number of file systems to handle

typedef struct {
    char device[256];
    char mount_point[256];
    char fs_type[64];
    unsigned long long total;
    unsigned long long used;
    unsigned long long available;
} FileSystemInfo;

int compare_fs(const void *a, const void *b) {
    FileSystemInfo *fsA = (FileSystemInfo *)a;
    FileSystemInfo *fsB = (FileSystemInfo *)b;
    return (fsB->available - fsA->available);  // Sort in descending order of total space
}

void get_file_system_stats() {
    FILE *fp = fopen("/proc/mounts", "r");
    if (!fp) {
        perror("Failed to open /proc/mounts");
        return;
    }

    FileSystemInfo fs_list[MAX_FS];
    int fs_count = 0;

    char device[256], mount_point[256], fs_type[64], options[256];
    int dump, pass;

    while (fscanf(fp, "%255s %255s %63s %255s %d %d", 
                  device, mount_point, fs_type, options, &dump, &pass) != EOF) {
        struct statvfs stat;
        if (statvfs(mount_point, &stat) == 0) {
            FileSystemInfo fs_info;
            strncpy(fs_info.device, device, sizeof(fs_info.device) - 1);
            strncpy(fs_info.mount_point, mount_point, sizeof(fs_info.mount_point) - 1);
            strncpy(fs_info.fs_type, fs_type, sizeof(fs_info.fs_type) - 1);
            fs_info.total = stat.f_blocks * stat.f_frsize;
            fs_info.available = stat.f_bavail * stat.f_frsize;
            fs_info.used = fs_info.total - fs_info.available;

            fs_list[fs_count++] = fs_info;

            // Limit the number of file systems stored to MAX_FS
            if (fs_count >= MAX_FS) {
                break;
            }
        }
    }

    fclose(fp);

    // Sort the file systems by total space
    qsort(fs_list, fs_count, sizeof(FileSystemInfo), compare_fs);

    // Print the top 4 file systems
    printf("\033[1;33m");
    printf("\033[%d;1H %-20s %-30s %-20s %-15s %-10s %-20s\n", FILE_SYSTEM_POS+1,
           "Device", "Directory", "Type", "Total (GB)", "Used (GB)", "Available (GB)");
    printf("\033[%d;1H-------------------------------------------------------------------------------------------------------------------------\n", FILE_SYSTEM_POS + 2);

    printf("\033[1;37m");
    for (int i = 0; i < 4 && i < fs_count; i++) {
        printf("%-20s %-30s %-20s %-15.2f %-10.2f %-20.2f\n", 
               fs_list[i].device, fs_list[i].mount_point, fs_list[i].fs_type, 
               fs_list[i].total / (1024.0 * 1024 * 1024), 
               fs_list[i].used / (1024.0 * 1024 * 1024), 
               fs_list[i].available / (1024.0 * 1024 * 1024));
    }
    printf("\033[0m");
}
