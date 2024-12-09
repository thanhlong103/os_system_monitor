#include <stdio.h>
#include "ui.h"
#include "cpu.h"

void print_static_ui() {
    // Enable alternate screen and clear it
    printf("\033[?1049h\033[H");

    // Title Header
    printf("\033[1;44;97m%-50s\033[0m\n", " SYSTEM MONITOR ");
    printf("\033[1;37m%-50s\033[0m\n", "Real-time System Overview");
    printf("==============================================\n");

    // Static sections
    printf("\033[%d;1H\033[1;36mTask Statistics:\033[0m\n", TASK_STATS_POS);
    printf("\033[%d;1H\033[1;34mCPU INFORMATION:\033[0m\n", CPU_INFO_POS);
    printf("\033[%d;1H\033[1;34mMEMORY INFORMATION:\033[0m\n", MEM_INFO_POS);
    printf("\033[%d;1H\033[1;34mPROCESSES BY CPU USAGE:\033[0m\n", PROC_INFO_POS);
}

void update_task_stats(int running, int sleeping, int stopped, int zombie, int total) {
    printf("\033[%d;1HTotal Processes: %d\n", TASK_STATS_POS + 1, total);
    printf("\033[%d;1HRunning: %d\n", TASK_STATS_POS + 2, running);
    printf("\033[%d;1HSleeping: %d\n", TASK_STATS_POS + 3, sleeping);
    printf("\033[%d;1HStopped: %d\n", TASK_STATS_POS + 4, stopped);
    printf("\033[%d;1HZombie: %d\n", TASK_STATS_POS + 5, zombie);
    printf("==============================================\n");
}

void update_cpu_info(double cpu_usage, double user_pct, double system_pct, double idle_pct,
                     double iowait_pct, double irq_pct, double softirq_pct, double steal_pct) {
    printf("\033[%d;1HTotal CPU Usage: \033[1;32m%.1f%%\033[0m\n", CPU_INFO_POS + 1, cpu_usage);
    printf("\033[%d;1HUsage Breakdown: \033[1;33mus=%.1f%% sy=%.1f%% id=%.1f%% io=%.1f%% hi=%.1f%% si=%.1f%% st=%.1f%%\033[0m\n",
           CPU_INFO_POS + 2, user_pct, system_pct, idle_pct, iowait_pct, irq_pct, softirq_pct, steal_pct);
}

void update_memory_info(long total_mem, long used_mem, long mem_free, long mem_cached, long mem_buffers) {
    printf("\033[%d;1HTotal Memory: \033[1;32m%.2f MB\033[0m\n", MEM_INFO_POS + 1, total_mem / 1024.0);
    printf("\033[%d;1HUsed Memory: \033[1;31m%.2f MB\033[0m\n", MEM_INFO_POS + 2, used_mem / 1024.0);
    printf("\033[%d;1HFree Memory: \033[1;31m%.2f MB\033[0m\n", MEM_INFO_POS + 3, mem_free / 1024.0);
    printf("\033[%d;1HBuffer Memory: \033[1;31m%.2f MB\033[0m\n", MEM_INFO_POS + 4, mem_buffers / 1024.0);
    printf("\033[%d;1HCache Memory: \033[1;31m%.2f MB\033[0m\n", MEM_INFO_POS + 5, mem_cached / 1024.0);
    printf("\033[%d;1HMemory Usage: \033[1;36m%.2f%%\033[0m\n", MEM_INFO_POS + 6, (double)used_mem / total_mem * 100.0);
    printf("==============================================\n");
}

void update_process_info() {
    list_processes();
}
