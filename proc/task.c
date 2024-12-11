#include <stdio.h>      // For printf, perror, fopen, fgets, fscanf, etc.
#include <dirent.h>     // For opendir, readdir, closedir, struct dirent
#include "task.h"
#include <ctype.h>

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

