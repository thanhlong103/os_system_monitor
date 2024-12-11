# OS System Monitor

This is a terminal-based system monitor designed to provide an overview of the system's performance. It display informations about CPU usage, memory, tasks, IO, and processes. For now, it just supports full-screen terminal view when running.

## Features
1. General informations of Tasks, CPU and Memory
2. Detail informations of each process (PID, User, CPU Usage (%), ...)
3. File System 

## Installation

Follow these steps to compile the project:

1. Change the directory to proc:
```
   cd os_system_monitor/proc
```

2. Compile the source files:
```
    gcc sysmon.c ui.c cpu.c task.c memory.c io.c process.c file_system.c -o sysmon
```

## Usage

After compiling, you can run the system monitor with the following command:

```
    ./sysmon
```
The sysmon will run by default sampling time is 1 second. If you want to change the sampling time, just add it after the ```./sysmon```, for example:
```
    ./sysmon 3
```