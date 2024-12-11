typedef struct {
    int pid;
    char user[256];
    char name[256];
    char priority[16];
    double cpu_usage;
    double mem_usage;
    char status[1];
    int nice;
    long virtual_mem;
    long res_mem;
    long shared_mem;
} ProcessInfo;

typedef struct {
    int pid;
    long prev_total_time;
    double prev_process_uptime;
} PrevProcessData;

void list_processes();
int compare_cpu_usage(const void *a, const void *b);
void update_prev_data(int pid, long total_time, double process_uptime);