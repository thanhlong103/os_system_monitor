#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/jiffies.h>
#include <linux/timer.h>
#include <linux/sched/signal.h>
#include <linux/sched.h>

// Module information
MODULE_LICENSE("GPL");
MODULE_AUTHOR("---");
MODULE_DESCRIPTION("Module to list all PIDs of running processes in real-time");
MODULE_VERSION("1.2");

// Timer object
static struct timer_list my_timer;

// Timer callback function
static void timer_callback(struct timer_list *timer)
{
    struct task_struct *task;

    printk(KERN_INFO "Listing tasks:\n");
    printk(KERN_INFO "INITIALIZED\n");
    for_each_process(task) {
        printk(KERN_INFO "PID: %d, Name: %s, Parent PID: %d\n", 
               task->pid, task->comm ,task->parent->pid);
    }
}


// Module initialization function
static int __init list_pids_init(void)
{
    // Initialize and start the timer
    timer_setup(&my_timer, timer_callback, 0);
    mod_timer(&my_timer, jiffies + msecs_to_jiffies(1000)); // Start in 1000ms

    printk(KERN_INFO "Module loaded: Timer started for task listing.\n");
    return 0; // Successfully loaded
}

// Module cleanup function
static void __exit list_pids_exit(void)
{
    // Remove the timer
    del_timer(&my_timer);
    printk(KERN_INFO "Module removed.\n");
}

module_init(list_pids_init);
module_exit(list_pids_exit);
