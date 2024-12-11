#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void get_disk_io(int pid, unsigned long long *read_bytes, unsigned long long *write_bytes) {

    char path[256];
    snprintf(path, sizeof(path), "/proc/%d/io", pid);

    FILE *fp = fopen(path, "r");
    if (!fp) {
        *read_bytes = 0;
        *write_bytes = 0;
        return;
    }

    char line[256];
    while (fgets(line, sizeof(line), fp)) {
        if (strncmp(line, "read_bytes:", 11) == 0) {
            sscanf(line + 11, "%llu", read_bytes);
        } else if (strncmp(line, "write_bytes:", 12) == 0) {
            sscanf(line + 12, "%llu", write_bytes);
        }
    }
    fclose(fp);
}