#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/sched/signal.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/jiffies.h>
#include <linux/delay.h>
#include <linux/slab.h> // For kmalloc and kfree

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Your Name");
MODULE_DESCRIPTION("A kernel module to print CPU usage of each process.");
MODULE_VERSION("1.0");

#define SAMPLING_DELAY_MS 1000  // Time delay in milliseconds
#define MAX_PROCESSES 512      // Limit the number of processes tracked

// Function to calculate and display CPU usage
static int show_cpu_usage(struct seq_file *m, void *v) {
    struct task_struct *task;
    unsigned long jiffies_t1, jiffies_t2;

    // Dynamically allocate memory for process snapshots
    struct {
        unsigned long utime;
        unsigned long stime;
        pid_t pid;
        char name[TASK_COMM_LEN];
    } *process_snapshot;

    process_snapshot = kmalloc_array(MAX_PROCESSES, sizeof(*process_snapshot), GFP_KERNEL);
    if (!process_snapshot) {
        seq_printf(m, "Memory allocation failed.\n");
        return -ENOMEM;
    }

    size_t process_count = 0;

    // First sampling
    jiffies_t1 = get_jiffies_64();
    for_each_process(task) {
        if (process_count < MAX_PROCESSES) {
            process_snapshot[process_count].utime = task->utime;
            process_snapshot[process_count].stime = task->stime;
            process_snapshot[process_count].pid = task->pid;
            get_task_comm(process_snapshot[process_count].name, task);
            process_count++;
        }
    }

    // Wait for a delay (sampling interval)
    msleep(SAMPLING_DELAY_MS);

    // Second sampling and calculation
    jiffies_t2 = get_jiffies_64();
    seq_printf(m, "%-30s%-10s%-10s\n", "Process Name", "PID", "CPU %%");

    for (size_t i = 0; i < process_count; i++) {
        task = find_task_by_vpid(process_snapshot[i].pid);
        if (task) {
            unsigned long utime_delta = task->utime - process_snapshot[i].utime;
            unsigned long stime_delta = task->stime - process_snapshot[i].stime;
            unsigned long cpu_time_delta = utime_delta + stime_delta;

            unsigned long jiffies_delta = jiffies_t2 - jiffies_t1;

            unsigned long cpu_usage = (jiffies_delta > 0) ? (cpu_time_delta * 100) / jiffies_delta : 0;

            seq_printf(m, "%-30s%-10d%-10lu\n", process_snapshot[i].name, process_snapshot[i].pid, cpu_usage);
        }
    }

    kfree(process_snapshot); // Free allocated memory
    return 0;
}

// File operations for proc file
static int cpu_usage_open(struct inode *inode, struct file *file) {
    return single_open(file, show_cpu_usage, NULL);
}

static const struct proc_ops cpu_usage_fops = {
    .proc_open = cpu_usage_open,
    .proc_read = seq_read,
    .proc_lseek = seq_lseek,
    .proc_release = single_release,
};

// Module initialization
static int __init cpu_usage_init(void) {
    proc_create("cpu_usage", 0, NULL, &cpu_usage_fops);
    printk(KERN_INFO "CPU usage module loaded.\n");
    return 0;
}

// Module exit
static void __exit cpu_usage_exit(void) {
    remove_proc_entry("cpu_usage", NULL);
    printk(KERN_INFO "CPU usage module unloaded.\n");
}

module_init(cpu_usage_init);
module_exit(cpu_usage_exit);
