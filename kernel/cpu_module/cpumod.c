#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/kernel_stat.h>  // For kernel_cpustat and CPUTIME_* macros
#include <linux/smp.h>          // For for_each_online_cpu
#include <linux/jiffies.h>      // For jiffies and HZ constant

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Your Name");
MODULE_DESCRIPTION("Retrieve and compare total CPU time with /proc/stat");
MODULE_VERSION("1.0");

// Function to retrieve total CPU time in jiffies
static void get_cpu_time(void)
{
    unsigned int cpu;
    u64 total_user_time = 0, total_system_time = 0, total_idle_time = 0;
    u64 total_iowait_time = 0, total_irq_time = 0, total_softirq_time = 0;
    u64 total_steal_time = 0, total_time = 0;

    // Loop through all online CPUs
    for_each_online_cpu(cpu) {
        struct kernel_cpustat *kstat = &kcpustat_cpu(cpu);

        // Accumulate the times for all states across the CPU
        total_user_time += kstat->cpustat[CPUTIME_USER]/10000000;
        total_system_time += kstat->cpustat[CPUTIME_SYSTEM]/10000000;
        total_idle_time += kstat->cpustat[CPUTIME_IDLE]/10000000;
        total_iowait_time += kstat->cpustat[CPUTIME_IOWAIT]/10000000;
        total_irq_time += kstat->cpustat[CPUTIME_IRQ]/10000000;
        total_softirq_time += kstat->cpustat[CPUTIME_SOFTIRQ]/10000000;
        total_steal_time += kstat->cpustat[CPUTIME_STEAL]/10000000;

        // Sum the total time (user + system + idle + iowait + irq + softirq + steal)
        total_time += (kstat->cpustat[CPUTIME_USER] + kstat->cpustat[CPUTIME_SYSTEM] + 
                      kstat->cpustat[CPUTIME_IDLE] + kstat->cpustat[CPUTIME_IOWAIT] + 
                      kstat->cpustat[CPUTIME_IRQ] + kstat->cpustat[CPUTIME_SOFTIRQ] + 
                      kstat->cpustat[CPUTIME_STEAL])/10000000;
    }

    // Print the total CPU time in jiffies without converting to seconds
    pr_info("Total CPU Time: %llu jiffies\n", total_time);

    pr_info("Total CPU Usage: %llu   \n", total_idle_time*100/total_time);

    // Print each component as well
    pr_info("User Time: %llu, System Time: %llu, Idle Time: %llu, I/O Wait Time: %llu, IRQ Time: %llu, SoftIRQ Time: %llu, Steal Time: %llu\n", 
            total_user_time, total_system_time, total_idle_time, total_iowait_time, total_irq_time, total_softirq_time, total_steal_time);
}

static int __init get_cpu_time_init(void)
{
    pr_info("Initializing CPU time retrieval module\n");

    // Call the function to retrieve CPU time
    get_cpu_time();

    return 0;
}

static void __exit get_cpu_time_exit(void)
{
    pr_info("Exiting CPU time retrieval module\n");
}

module_init(get_cpu_time_init);
module_exit(get_cpu_time_exit);
