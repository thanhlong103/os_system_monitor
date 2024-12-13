# OS System Monitor

```
Cao Hoang Dung - 210027
Nguyen Thanh Long - 210085
Nguyen Duc Manh - 210230
```

Each foler ```kernel``` and ```proc``` was used to implement system monitor using its corresponding approach: getting processes' information directly from the kernel and from ```proc ``` file system. However, we tried with the ```kernel``` approach and found it quite difficult, so we only proceed with the ```proc``` approach.

## Features
1. General informations of Tasks, CPU and Memory
2. Detail informations of each process (PID, User, CPU Usage (%), ...)
3. File System 

## Compile

Follow these steps to compile the project:

1. Change the directory to proc:
```
   cd os_system_monitor/proc
```

2. Compile the source files:
```
    gcc sysmon.c ui.c cpu.c task.c memory.c io.c process.c filesystem.c -o sysmon
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

If the layout appears broken or does not display properly, try zooming out the terminal by using the following methods:
- **From the menu**: Go to `View` -> `Zoom Out`.
- **Using a keyboard shortcut**: Press `Ctrl + -` to zoom out.