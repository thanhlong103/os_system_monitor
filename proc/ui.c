#include <stdio.h>
#include "ui.h"
#include "cpu.h"

#include "process.h"

void print_static_ui()
{
    // Enable alternate screen and clear it
    printf("\033[?1049h\033[H");

    // Title Header
    printf("\033[1;44;97m%-50s\033[0m\n", " SYSTEM MONITOR ");
    printf("==============================================\n");

    // Static sections
    printf("\033[%d;1H\033[1;36mTask Statistics:\033[0m\n", TASK_STATS_POS);
    printf("\033[%d;1H\033[1;36mCPU INFORMATION:\033[0m\n", CPU_INFO_POS);
    printf("\033[%d;1H\033[1;36mMEMORY AND SWAP:\033[0m\n", MEM_INFO_POS);
    printf("\033[%d;1H\033[1;36mFILE SYSTEM:\033[0m\n", FILE_SYSTEM_POS);
}

void update_task_stats(int running, int sleeping, int stopped, int zombie, int total)
{
    printf("\033[%d;1HTasks: \033[1m%d\033[0m, Running: \033[1m%d\033[0m, Sleeping: \033[1m%d\033[0m, Stopped: \033[1m%d\033[0m, Zombie: \033[1m%d\033[0m\n",
           TASK_STATS_POS + 1,
           total, running, sleeping, stopped, zombie);
}

void update_cpu_info(double cpu_usage, double user_pct, double system_pct, double idle_pct,
                     double iowait_pct, double irq_pct, double softirq_pct, double steal_pct)
{
    printf("\033[%d;1HTotal CPU Usage: \033[1;32m%.1f%%\033[0m\n", CPU_INFO_POS + 1, cpu_usage);
    printf("\033[%d;1HUsage Breakdown: \033[1;33mus=%.1f%% sy=%.1f%% id=%.1f%% io=%.1f%% hi=%.1f%% si=%.1f%% st=%.1f%%\033[0m\n",
           CPU_INFO_POS + 2, user_pct, system_pct, idle_pct, iowait_pct, irq_pct, softirq_pct, steal_pct);
}

void update_memory_info(long total_mem, long used_mem, long available_mem, long mem_free, long mem_cached, long mem_buffers, long kreclaimable)
{
    printf("\033[%d;1HMemory: \033[1m%.2f MB\033[0m, Used: \033[1m%.2f MB\033[0m, Available: \033[1m%.2f MB\033[0m, Free: \033[1m%.2f\033[0m MB, Buff/Cache: \033[1m%.2f MB\n",
        MEM_INFO_POS + 2,
        total_mem / 1024.0,                                  // Total Memory in green
        used_mem / 1024.0,
        available_mem/ 1024.0,                                   // Used Memory in red
        mem_free / 1024.0,                                   // Free Memory in default color
        (mem_buffers + mem_cached + kreclaimable) / 1024.0); // Buff/Cache Memory in default color

    printf("\033[%d;1HMemory Usage: \033[1;32m%.2f%%\033[0m\n", MEM_INFO_POS + 1, (double)used_mem / total_mem * 100.0);

}

void update_swap_info(long total_swap, long free_swap)
{
    printf("\033[%d;1HSwap: \033[1m%.2f MB\033[0m, Used Swap: \033[1m%.2f MB\033[0m, Free Swap: \033[1m%.2f MB\n",
           MEM_INFO_POS + 3,
           total_swap / 1024.0,                               
           (total_swap - free_swap)/ 1024.0,                                 
           free_swap / 1024.0); 
    printf("\n");
}

void update_process_info()
{
    list_processes();
}
