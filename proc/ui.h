#ifndef UI_H
#define UI_H

// Line positions for different sections
#define TASK_STATS_POS 4
#define CPU_INFO_POS 7
#define MEM_INFO_POS 11
#define PROC_INFO_POS 16
#define FILE_SYSTEM_POS 38

void print_static_ui();
void update_task_stats(int running, int sleeping, int stopped, int zombie, int total);
void update_cpu_info(double cpu_usage, double user_pct, double system_pct, double idle_pct,
                     double iowait_pct, double irq_pct, double softirq_pct, double steal_pct);
void update_memory_info(long total_mem, long used_mem, long available_mem, long mem_free, long mem_cached, long mem_buffers, long kreclaimable);
void update_swap_info(long total_swap, long free_swap);

void update_process_info();

#endif
