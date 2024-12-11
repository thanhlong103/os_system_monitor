#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <time.h>
#include "task.h"
#include "ui.h"
#include "memory.h"
#include "cpu.h"

int main() {
    print_static_ui();

    int running, sleeping, stopped, zombie, total;
    long total_mem, used_mem, mem_free, mem_cached, mem_buffers, mem_available, kreclaimable;
    double user_pct, system_pct, idle_pct, iowait_pct, irq_pct, softirq_pct, steal_pct;

    while (1) {
        get_task_info(&running, &sleeping, &stopped, &zombie, &total);
        get_memory_info(&total_mem, &used_mem, &mem_buffers, &mem_cached, &mem_free, &mem_available, &kreclaimable);

        double cpu_usage = get_cpu_usage(&user_pct, &system_pct, &idle_pct, &iowait_pct, &irq_pct, &softirq_pct, &steal_pct);

        update_task_stats(running, sleeping, stopped, zombie, total);
        update_cpu_info(cpu_usage, user_pct, system_pct, idle_pct, iowait_pct, irq_pct, softirq_pct, steal_pct);
        update_memory_info(total_mem, used_mem, mem_free, mem_cached, mem_buffers, kreclaimable);
        update_process_info();

        fflush(stdout); // Ensure immediate output
        sleep(1);       // Refresh every second
    }

    // Restore normal screen on exit
    printf("\033[?1049l");
    return 0;
}
