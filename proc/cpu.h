typedef struct {
    int pid;
    char user[256];
    char name[256];
    double cpu_usage;
    double mem_usage;
} ProcessInfo;

int get_core_count();
int get_thread_count();
// double get_cpu_usage(double *user_pct, double *system_pct, double *idle_pct, double *iowait_pct, double *irq_pct, double *softirq_pct, double *steal_pct);
void list_processes();
int compare_cpu_usage(const void *a, const void *b);