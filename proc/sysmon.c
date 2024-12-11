#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <time.h>
#include "task.h"
#include "ui.h"
#include "memory.h"
#include "cpu.h"
#include "process.h"
#include "filesystem.h"

int main(int argc, char *argv[]) {
    // Default sleep time in seconds
    int sleep_time = 1;

    // Check if the user provided a custom sleep time
    if (argc > 1) {
        sleep_time = atoi(argv[1]);

        // Check if the provided sleep time is a valid natural number
        if (sleep_time <= 0) {
            fprintf(stderr, "Invalid sleep time provided. Using default (1 second).\n");
            sleep_time = 1;  // Reset to default
        }
    }

    print_static_ui();

    int running, sleeping, stopped, zombie, total;
    long total_mem, used_mem, mem_free, mem_cached, mem_buffers, mem_available, kreclaimable;
    long total_swap, free_swap;

    double user_pct, system_pct, idle_pct, iowait_pct, irq_pct, softirq_pct, steal_pct;

    while (1) {
        clock_t start_calculation = clock();  // Start measuring time for each iteration

        get_task_info(&running, &sleeping, &stopped, &zombie, &total);
        get_memory_info(&total_mem, &used_mem, &mem_buffers, &mem_cached, &mem_free, &mem_available, &kreclaimable, &total_swap, &free_swap);

        double cpu_usage = get_cpu_usage(&user_pct, &system_pct, &idle_pct, &iowait_pct, &irq_pct, &softirq_pct, &steal_pct);
        update_task_stats(running, sleeping, stopped, zombie, total);
        update_cpu_info(cpu_usage, user_pct, system_pct, idle_pct, iowait_pct, irq_pct, softirq_pct, steal_pct);
        update_memory_info(total_mem, used_mem, mem_available, mem_free, mem_cached, mem_buffers, kreclaimable);
        update_swap_info(total_swap, free_swap);
        update_process_info();

        get_file_system_stats();

        // clock_t end_calculation = clock();  // End measuring time for each iteration
        // double calculation_time = ((double)(end_calculation - start_calculation)) / CLOCKS_PER_SEC;  // Time in seconds

        // // Print the runtime and calculation time
        // printf("Calculation Time: %.6f s", calculation_time); // Overwrite previous output

        fflush(stdout); // Ensure immediate output
        sleep(sleep_time);  // Refresh based on user-defined sleep time
    }

    // Restore normal screen on exit
    printf("\033[?1049l");
    return 0;
}
