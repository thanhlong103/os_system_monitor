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
    char fs_directory[64];
    unsigned long long total;
    unsigned long long used;
    unsigned long long available;
} FileSystemInfo;

void replace_escaped_spaces_with_blank(char *str) {
    char *src = str, *dst = str;
    while (*src) {
        // Check if we encounter the escape sequence '\040'
        if (*src == '\\' && *(src + 1) == '0' && *(src + 2) == '4' && *(src + 3) == '0') {
            // Replace '\040' with a space
            *dst++ = ' ';  // Add a blank space instead
            src += 4;      // Skip the 4 characters ('\040')
        } else {
            *dst++ = *src++;  // Copy the character as is
        }
    }
    *dst = '\0';  // Null-terminate the string
}

int is_invalid_fs(const char *fs_directory, const char *fs_type) {
    const char *invalid_fs[] = {
        "/dev", "/run", "/sys", "/snap", "/proc", NULL
    };
    for (int i = 0; invalid_fs[i] != NULL; i++) {
        if (strstr(fs_directory, invalid_fs[i]) != NULL) {
            return 1;  // Return 1 if fs_directory contains any of the invalid paths
        }
    }
    return 0; 
}
 
int compare_fs(const void *a, const void *b) {
    FileSystemInfo *fsA = (FileSystemInfo *)a;
    FileSystemInfo *fsB = (FileSystemInfo *)b;
    return (fsB->available - fsA->available); 
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
        if (is_invalid_fs(mount_point, fs_type)) {
            continue;
        }
                    replace_escaped_spaces_with_blank(device);
            replace_escaped_spaces_with_blank(mount_point);
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
    printf("\033[%d;1H %-30s %-50s %-20s %-15s %-20s\n", FILE_SYSTEM_POS+1,
           "Device", "Directory", "Type", "Total (GB)", "Available (GB)");
    printf("\033[%d;1H-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------\n", FILE_SYSTEM_POS + 2);

    printf("\033[1;37m");
    for (int i = 0; i < fs_count && i < fs_count; i++) {
        printf("%-30s %-50s %-20s %-15.2f %-20.2f\n", 
               fs_list[i].device, fs_list[i].mount_point, fs_list[i].fs_type, 
               fs_list[i].total / (1000.0* 1000 * 1000), 
               fs_list[i].available / (1000.0 * 1000 * 1000));
    }
    printf("\033[0m");
}
