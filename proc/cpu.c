#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include "cpu.h"
#include <pwd.h>
#include <ctype.h>
#include "io.h"

// Function to calculate CPU usage with breakdown of percentages
double get_cpu_usage(double *user_pct, double *system_pct, double *idle_pct, double *iowait_pct, double *irq_pct, double *softirq_pct, double *steal_pct) {
    static long prev_total = 0;
    static long prev_user = 0;
    static long prev_system = 0;
    static long prev_idle = 0;
    static long prev_iowait = 0;
    static long prev_irq = 0;
    static long prev_softirq = 0;
    static long prev_steal = 0;
    long user, nice, system, idle, iowait, irq, softirq, steal;
    long total, totald, userd, systemd, idled, iowaitd, irqd, softirqd, steald;
    double cpu_usage;

    FILE *fp = fopen("/proc/stat", "r");
    if (fp == NULL) {
        perror("Failed to read /proc/stat");
        return -1.0;
    }

    // Read CPU time values from /proc/stat
    if (fscanf(fp, "cpu %ld %ld %ld %ld %ld %ld %ld %ld", 
               &user, &nice, &system, &idle, &iowait, &irq, &softirq, &steal) != 8) {
        fclose(fp);
        return -1.0;
    }
    fclose(fp);

    // Calculate current total and idle times
    long current_user = user + nice;
    long current_total = user + nice + system + iowait + irq + softirq + steal + idle;

    // If previous values are zero (first call), initialize them
    if (prev_total == 0 && prev_idle == 0) {
        prev_total = current_total;
        prev_user = current_user;
        prev_idle = idle;
        prev_system = system;
        prev_iowait = iowait;
        prev_irq = irq;
        prev_softirq = softirq;
        prev_steal = steal;
        return 0.0; // No usage to calculate on the first call
    }

    // Calculate deltas
    totald = current_total - prev_total;
    userd = current_user - prev_user;
    systemd = system - prev_system;
    idled = idle - prev_idle;
    iowaitd = iowait - prev_iowait;
    irqd = irq - prev_irq;
    softirqd = softirq - prev_softirq;
    steald = steal - prev_steal;

    // Update previous values for the next calculation
    prev_total = current_total;
    prev_user = current_user;
    prev_idle = idle;
    prev_system = system;
    prev_iowait = iowait;
    prev_irq = irq;
    prev_softirq = softirq;
    prev_steal = steal;

    // Calculate overall CPU usage percentage
    cpu_usage = (totald - idled) / (double)totald * 100.0;

    // Calculate breakdown percentages
    *user_pct = userd / (double)totald * 100.0;
    *system_pct = systemd / (double)totald * 100.0;
    *idle_pct = idled / (double)totald * 100.0;
    *iowait_pct = iowaitd / (double)totald * 100.0;
    *irq_pct = irqd / (double)totald * 100.0;
    *softirq_pct = softirqd / (double)totald * 100.0;
    *steal_pct = steald / (double)totald * 100.0;

    return cpu_usage;
}