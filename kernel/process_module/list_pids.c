#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/sched.h>
#include <linux/cred.h>
#include <linux/uidgid.h>
#include <linux/slab.h>

// Module information
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Your Name");
MODULE_DESCRIPTION("Module to list all PIDs of running processes with their usernames and parent PID");
MODULE_VERSION("1.5");

// Function to get username from UID
static char *get_username_from_uid(kuid_t uid)
{
    char *username;
    username = kmalloc(32, GFP_KERNEL);
    if (!username)
        return "Unknown";

    // Placeholder logic for fetching the username (not accurate for non-root UIDs)
    if (uid_eq(uid, GLOBAL_ROOT_UID)) {
        snprintf(username, 32, "root");
    } else {
        snprintf(username, 32, "UID:%d", uid.val);
    }

    return username;
}

// Function to list processes
static void list_processes(void)
{
    struct task_struct *task;
    char *username;

    printk(KERN_INFO "Listing tasks:\n");
    for_each_process(task) {
        username = get_username_from_uid(task_uid(task));

        printk(KERN_INFO "PID: %d, Name: %s, User: %s, Parent PID: %d\n",
               task->pid, task->comm, username, task->parent->pid);

        kfree(username);
    }
}

// Module initialization function
static int __init list_pids_init(void)
{
    printk(KERN_INFO "Module loaded: Listing tasks.\n");
    list_processes();
    return 0; // Successfully loaded
}

// Module cleanup function
static void __exit list_pids_exit(void)
{
    printk(KERN_INFO "Module removed.\n");
}

module_init(list_pids_init);
module_exit(list_pids_exit);
