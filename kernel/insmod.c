#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdbool.h> // For the 'bool' type
#include <sys/stat.h> // For struct stat and stat()

#define MODULE_NAME "cpumod.ko" // Replace with the actual module name

bool file_exists(const char *path) {
    struct stat buffer;
    return (stat(path, &buffer) == 0);
}

void insert_module(const char *module_name) {
    char command[256];
    snprintf(command, sizeof(command), "sudo insmod cpu_module/%s", module_name);
    int result = system(command);
    if (result == 0) {
        printf("Module %s inserted successfully.\n", module_name);
    } else {
        printf("Failed to insert module %s.\n", module_name);
    }
}

void remove_module(const char *module_name) {
    char command[256];
    // snprintf(command, sizeof(command), "lsmod | grep %s", module_name);
    // if (system(command) == 0) { // Module is loaded
    snprintf(command, sizeof(command), "sudo rmmod %s", module_name);
    int result = system(command);
    if (result == 0) {
        printf("Module %s removed successfully.\n", module_name);
    } else {
        printf("Failed to remove module %s.\n", module_name);
    }

}

void print_cpu_usage() {
    // Read from dmesg to print kernel log messages
    printf("Fetching CPU usage...\n");
    system("sudo dmesg | grep 'Total CPU Usage'");
}

int main() {
    while (1) {
        // Insert the module
        insert_module(MODULE_NAME);

        // Print CPU usage
        print_cpu_usage();

        // Wait for a few seconds before removing and reinserting the module
        sleep(1);

        // Remove the module
        remove_module(MODULE_NAME);

        // Wait for a few seconds before inserting the module again
        sleep(1);
    }

    return 0;
}
