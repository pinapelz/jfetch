#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <winsock2.h>
#include <pdh.h>
#include <stdint.h>
#include <windows.h>
#include <psapi.h>
#include <sysinfoapi.h>
#include <io.h>
#include <direct.h>
#define usleep(x) Sleep((x) / 1000)
#define popen _popen
#define pclose _pclose
#else
#include <signal.h>
#include <unistd.h>
#include <sys/utsname.h>
#include <sys/sysinfo.h>
#include <sys/statvfs.h>
#endif

#include "jorb.c" // Jorb animation object
#include "jfetch.h"

// Platform-specific implementations
#ifdef _WIN32
#define PATH_MAX 260
#define BUFFERSIZE 256
#endif

// IMPLEMENTED:
// Username [x]
// Host [x]
// Datetime [x]
// OS [x]
// Kernel [x]
// Desktop [x]
// Shell [x]
// Terminal [x]
// CPU [x]
// CPU usage [x]
// Memory [x]
// Swap [x]
// Disk [x]
// Processes [x]
// Uptime [x]
// Battery [x]

static system_stats sysstats = { 0 };
animation_object *jorb = NULL;

char *yield_frame(animation_object *ao) {
    char *frame = ao->frames[ao->current_frame];
    ao->current_frame = (ao->current_frame + 1) % ao->frame_count;
    return frame;
}

void fetch_user_name(char *user_name) {
    NULL_RETURN(user_name);
    strncpy(user_name, "Unknown", BUFFERSIZE);

#ifdef _WIN32
    char *value = getenv("USERNAME");
    if (value)
        strncpy(user_name, value, BUFFERSIZE);
#else
    char *value = getenv("USER");
    if (value)
        strncpy(user_name, value, BUFFERSIZE);
#endif
}

void fetch_host_name(char *host_name) {
    NULL_RETURN(host_name);
    strncpy(host_name, "Unknown", BUFFERSIZE);

    char buffer[BUFFERSIZE] = { 0 };
    if (!gethostname(buffer, BUFFERSIZE)) {
        buffer[BUFFERSIZE - 1] = '\0';
        strncpy(host_name, buffer, BUFFERSIZE);
    }
}

void fetch_datetime(char *datetime) {
    NULL_RETURN(datetime);
    strncpy(datetime, "Unknown", BUFFERSIZE);

    time_t dt;
    if (time(&dt) != -1) {
        struct tm *local = localtime(&dt);
        strftime(datetime, BUFFERSIZE, "%Y-%m-%d %H:%M:%S", local);
    }
}

void fetch_os_name(char *os_name) {
    NULL_RETURN(os_name);
    strncpy(os_name, "Unknown", BUFFERSIZE);

#ifdef _WIN32
    HKEY hKey;
    char productName[BUFFERSIZE] = {0};
    char releaseId[BUFFERSIZE] = {0};
    char displayVersion[BUFFERSIZE] = {0};
    DWORD size;
    char buildNumberStr[BUFFERSIZE] = {0};

    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                    "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion",
                    0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        size = BUFFERSIZE;
        RegQueryValueEx(hKey, "CurrentBuildNumber", NULL, NULL, (LPBYTE)buildNumberStr, &size);
        int buildNumber = atoi(buildNumberStr);
        size = BUFFERSIZE;
        RegQueryValueEx(hKey, "ProductName", NULL, NULL, (LPBYTE)productName, &size);
        if (buildNumber >= 22000 && strstr(productName, "Windows 10") != NULL) {
            char *pos = strstr(productName, "Windows 10");
            pos[8] = '1';
            pos[9] = '1';
        }
        size = BUFFERSIZE;
        RegQueryValueEx(hKey, "DisplayVersion", NULL, NULL, (LPBYTE)displayVersion, &size);
        if (strlen(displayVersion) == 0) {
            size = BUFFERSIZE;
            RegQueryValueEx(hKey, "ReleaseId", NULL, NULL, (LPBYTE)releaseId, &size);
        }

        if (strlen(displayVersion) > 0) {
            snprintf(os_name, BUFFERSIZE, "%s %s", productName, displayVersion);
        } else if (strlen(releaseId) > 0) {
            snprintf(os_name, BUFFERSIZE, "%s %s", productName, releaseId);
        } else {
            strncpy(os_name, productName, BUFFERSIZE);
        }

        RegCloseKey(hKey);
    }
#else
    FILE *f = fopen("/etc/os-release", "r");
    if (!f)
        return;

    char buffer[BUFFERSIZE] = { 0 };
    char *tmp = NULL;
    while (fgets(buffer, BUFFERSIZE, f) != NULL) {
        if (!strncmp(buffer, "PRETTY_NAME=", 12)) {
            tmp = buffer + 12;
            if (tmp[0] == '\"') tmp++;
            size_t length = strlen(tmp);
            if (tmp[length - 1] == '\n') length--;
            if (tmp[length - 1] == '\"') length--;
            memcpy(os_name, tmp, length);
            break;
        }
    }

    fclose(f);
#endif
}

void fetch_kernel_version(char *kernel_version) {
    NULL_RETURN(kernel_version);
    strncpy(kernel_version, "Unknown", BUFFERSIZE);

#ifdef _WIN32
    char buildNumber[BUFFERSIZE] = {0};
    OSVERSIONINFOEX osvi;
    ZeroMemory(&osvi, sizeof(OSVERSIONINFOEX));
    osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
    GetVersionEx((OSVERSIONINFO*)&osvi);
    HKEY hKey;
    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                     "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion",
                     0, KEY_READ, &hKey) == ERROR_SUCCESS) {

        DWORD size = BUFFERSIZE;
        RegQueryValueEx(hKey, "CurrentBuildNumber", NULL, NULL, (LPBYTE)buildNumber, &size);
        RegCloseKey(hKey);
    }
    DWORD ubr = 0;
    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                     "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion",
                     0, KEY_READ, &hKey) == ERROR_SUCCESS) {

        DWORD size = sizeof(DWORD);
        RegQueryValueEx(hKey, "UBR", NULL, NULL, (LPBYTE)&ubr, &size);

        char displayVersion[BUFFERSIZE] = {0};
        size = BUFFERSIZE;
        RegQueryValueEx(hKey, "DisplayVersion", NULL, NULL, (LPBYTE)displayVersion, &size);

        if (strlen(displayVersion) > 0) {
            snprintf(kernel_version, BUFFERSIZE, "WIN32_NT %d.%d.%s.%d (%s)",
                    osvi.dwMajorVersion, osvi.dwMinorVersion, buildNumber, ubr, displayVersion);
        } else {
            snprintf(kernel_version, BUFFERSIZE, "WIN32_NT %d.%d.%s.%d",
                    osvi.dwMajorVersion, osvi.dwMinorVersion, buildNumber, ubr);
        }

        RegCloseKey(hKey);
    } else {
        snprintf(kernel_version, BUFFERSIZE, "WIN32_NT %d.%d (Build %s)",
                osvi.dwMajorVersion, osvi.dwMinorVersion, buildNumber);
    }
#else
    struct utsname data;
    if (!uname(&data)) {
        strncpy(kernel_version, data.sysname, BUFFERSIZE);
        size_t length = strlen(kernel_version);
        if (length < BUFFERSIZE)
            kernel_version[length] = '-';
        strncpy(kernel_version + length + 1, data.release, BUFFERSIZE - length - 1);
    }
#endif
}

void fetch_desktop_name(char *desktop_name) {
    NULL_RETURN(desktop_name);
    strncpy(desktop_name, "Unknown", BUFFERSIZE);

#ifdef _WIN32
    OSVERSIONINFOEX osvi;
    ZeroMemory(&osvi, sizeof(OSVERSIONINFOEX));
    osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
    GetVersionEx((OSVERSIONINFO*)&osvi);

    char buildNumber[BUFFERSIZE] = {0};
    HKEY hKey;
    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                   "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion",
                   0, KEY_READ, &hKey) == ERROR_SUCCESS) {

        DWORD size = BUFFERSIZE;
        RegQueryValueEx(hKey, "CurrentBuildNumber", NULL, NULL, (LPBYTE)buildNumber, &size);
        RegCloseKey(hKey);
    }
    int buildNum = atoi(buildNumber);
    if (buildNum >= 22000) {
        strncpy(desktop_name, "Fluent", BUFFERSIZE);
    } else {
        strncpy(desktop_name, "Windows Explorer", BUFFERSIZE);
    }
#else
    char *desktop = getenv("XDG_SESSION_DESKTOP");
    char *session = getenv("XDG_SESSION_TYPE");

    if (desktop && session) {
        snprintf(desktop_name, BUFFERSIZE, "%s (%s)", desktop, session);
    }
#endif
}

void fetch_shell_name(char *shell_name) {
    NULL_RETURN(shell_name);
    strncpy(shell_name, "Unknown", BUFFERSIZE);

#ifdef _WIN32
    char *comspec = getenv("COMSPEC");
    if (comspec) {
        char *name = strrchr(comspec, '\\');
        if (name)
            strncpy(shell_name, name + 1, BUFFERSIZE);
        else
            strncpy(shell_name, comspec, BUFFERSIZE);
    }
#else
    char *value = getenv("SHELL");
    value = strrchr(value, '/');
    if (value)
        strncpy(shell_name, value + 1, BUFFERSIZE);
#endif
}

void fetch_terminal_name(char *terminal_name) {
    NULL_RETURN(terminal_name);
    strncpy(terminal_name, "Unknown", BUFFERSIZE);

#ifdef _WIN32
    char buffer[BUFFERSIZE] = { 0 };
    if (getenv("WT_SESSION")) {
        strncpy(terminal_name, "Windows Terminal", BUFFERSIZE);
    } else if (getenv("TERM_PROGRAM")) {
        strncpy(terminal_name, getenv("TERM_PROGRAM"), BUFFERSIZE);
    } else {
        OSVERSIONINFOEX osvi;
        ZeroMemory(&osvi, sizeof(OSVERSIONINFOEX));
        osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
        GetVersionEx((OSVERSIONINFO*)&osvi);

        DWORD buildNumber = 0;
        HKEY hKey;
        if (RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                      "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion",
                      0, KEY_READ, &hKey) == ERROR_SUCCESS) {
            DWORD size = sizeof(DWORD);
            DWORD ubr = 0;
            RegQueryValueEx(hKey, "UBR", NULL, NULL, (LPBYTE)&ubr, &size);

            char buildNumberStr[BUFFERSIZE];
            size = BUFFERSIZE;
            RegQueryValueEx(hKey, "CurrentBuildNumber", NULL, NULL, (LPBYTE)buildNumberStr, &size);
            buildNumber = atoi(buildNumberStr);

            snprintf(terminal_name, BUFFERSIZE, "Windows Console %d.%d.%d.%d",
                    osvi.dwMajorVersion, osvi.dwMinorVersion, buildNumber, ubr);

            RegCloseKey(hKey);
        } else {
            strncpy(terminal_name, "Windows Console", BUFFERSIZE);
        }
    }
#else
    char buffer[BUFFERSIZE] = { 0 };
    snprintf(buffer, BUFFERSIZE, "/proc/%d/stat", getppid());

    FILE *f = fopen(buffer, "r");
    if (!f)
        return;

    pid_t ppid;
    if (fscanf(f, "%*d %*s %*c %d", &ppid) != 1) {
        fclose(f);
        return;
    }
    fclose(f);

    snprintf(buffer, BUFFERSIZE, "/proc/%d/comm", ppid);
    f = fopen(buffer, "r");
    if (!f)
        return;
    if (fgets(buffer, BUFFERSIZE, f) != NULL) {
        strncpy(terminal_name, buffer, BUFFERSIZE);
    }
    fclose(f);
#endif
}

#ifdef _WIN32
static BOOL FileTimeToUInt64(const FILETIME *ft, uint64_t *out) {
    if (!ft || !out) return FALSE;
    *out = ((uint64_t)ft->dwHighDateTime << 32) | ft->dwLowDateTime;
    return TRUE;
}
#endif

void fetch_cpu_name(char *cpu_name) {
    NULL_RETURN(cpu_name);
    strncpy(cpu_name, "Unknown", BUFFERSIZE);

#ifdef _WIN32
    HKEY hKey;
    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                    "HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0",
                    0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        DWORD size = BUFFERSIZE;
        RegQueryValueEx(hKey, "ProcessorNameString", NULL, NULL, (LPBYTE)cpu_name, &size);
        RegCloseKey(hKey);
    }
#else
    FILE *f = fopen("/proc/cpuinfo", "r");
    if (!f)
        return;

    char buffer[BUFFERSIZE] = { 0 };
    char *tmp = NULL;
    while (fgets(buffer, BUFFERSIZE, f) != NULL) {
        if (!strncmp(buffer, "model name", 10)) {
            tmp = strstr(buffer, ": ") + 2;
            size_t length = strlen(tmp);
            if (tmp[length - 1] == '\n') length--;
            memcpy(cpu_name, tmp, length);
            break;
        }
    }

    fclose(f);
#endif
}

void fetch_cpu_usage(char *cpu_usage) {
    NULL_RETURN(cpu_usage);

#ifdef _WIN32
    static uint64_t _prevIdle = 0, _prevKernel = 0, _prevUser = 0;
    FILETIME idleTime, kernelTime, userTime;

    if (!GetSystemTimes(&idleTime, &kernelTime, &userTime)) {
        snprintf(cpu_usage, BUFFERSIZE, "GetSystemTimes Error");
        return;
    }

    uint64_t idle, kernel, user;
    if (!FileTimeToUInt64(&idleTime, &idle) ||
        !FileTimeToUInt64(&kernelTime, &kernel) ||
        !FileTimeToUInt64(&userTime, &user)) {
        snprintf(cpu_usage, BUFFERSIZE, "Time Conversion Error");
        return;
    }

    if (_prevIdle == 0 && _prevKernel == 0 && _prevUser == 0) {
        _prevIdle   = idle;
        _prevKernel = kernel;
        _prevUser   = user;
        snprintf(cpu_usage, BUFFERSIZE, "Measuring...");
        return;
    }
    uint64_t idleDiff   = idle   - _prevIdle;
    uint64_t kernelDiff = kernel - _prevKernel;
    uint64_t userDiff   = user   - _prevUser;
    uint64_t total = kernelDiff + userDiff;
    uint64_t busy  = total - idleDiff;

    double cpuPercent = total ? (busy * 100.0) / (double)total : 0.0;
    _prevIdle   = idle;
    _prevKernel = kernel;
    _prevUser   = user;

    snprintf(cpu_usage, BUFFERSIZE, "%.0f%%", cpuPercent);
#else
    strncpy(cpu_usage, "Unknown", BUFFERSIZE);

    FILE *f = fopen("/proc/stat", "r");
    if (!f)
        return;

    static size_t prev_total = 0, prev_idle = 0;
    char buffer[BUFFERSIZE];
    if (fgets(buffer, BUFFERSIZE, f) != NULL) {
        size_t user, nice, system, idle, iowait, irq, softirq, steal, guest, guest_nice;
        sscanf(buffer, "cpu %zd %zd %zd %zd %zd %zd %zd %zd %zd %zd",
            &user, &nice, &system, &idle, &iowait, &irq, &softirq, &steal, &guest, &guest_nice
        );
        size_t total = user + nice + system + idle + iowait + irq + softirq + steal + guest + guest_nice;
        if (prev_total > 0) {
            memset(cpu_usage, 0, BUFFERSIZE); // Clear the buffer
            snprintf(cpu_usage, BUFFERSIZE,
                "%.0f%%", (1 - (double)(idle - prev_idle) / (total - prev_total)) * 100
            );
        }
        prev_total = total;
        prev_idle = idle;
    }

    fclose(f);
#endif
}

void fetch_ram_usage(char *ram_usage) {
    NULL_RETURN(ram_usage);
    strncpy(ram_usage, "Unknown", BUFFERSIZE);

#ifdef _WIN32
    MEMORYSTATUSEX memInfo;
    memInfo.dwLength = sizeof(MEMORYSTATUSEX);
    GlobalMemoryStatusEx(&memInfo);

    DWORDLONG totalMem = memInfo.ullTotalPhys;
    DWORDLONG usedMem = totalMem - memInfo.ullAvailPhys;

    double totalGB = (double)totalMem / (1024 * 1024 * 1024);
    double usedGB = (double)usedMem / (1024 * 1024 * 1024);
    double percentage = 100.0 * usedMem / totalMem;

    snprintf(ram_usage, BUFFERSIZE, "%.2fGB / %.2fGB (%.0f%%)",
             usedGB, totalGB, percentage);
#else
    FILE *f = fopen("/proc/meminfo", "r");
    if (!f)
        return;

    char buffer[BUFFERSIZE] = { 0 };
    size_t total_kB = 0, used_kB = 0;
    while (fgets(buffer, BUFFERSIZE, f) != NULL) {
        if (total_kB && used_kB)
            break;
        sscanf(buffer, "MemTotal: %zd kB", &total_kB);
        sscanf(buffer, "MemAvailable: %zd kB", &used_kB);
    }

    if (total_kB && used_kB) {
        used_kB = total_kB - used_kB;
        snprintf(ram_usage, BUFFERSIZE,
            "%.2fGB / %.2fGB (%.0f%%)",
            (double)used_kB / 1024 / 1024,
            (double)total_kB / 1024 / 1024,
            (double)used_kB / total_kB * 100
        );
    }

    fclose(f);
#endif
}

void fetch_swap_usage(char *swap_usage) {
    NULL_RETURN(swap_usage);
    strncpy(swap_usage, "Unknown", BUFFERSIZE);

#ifdef _WIN32
    MEMORYSTATUSEX memInfo;
    memInfo.dwLength = sizeof(MEMORYSTATUSEX);
    if (GlobalMemoryStatusEx(&memInfo)) {
        DWORDLONG totalPhysical = memInfo.ullTotalPhys;
        DWORDLONG totalVirtual = memInfo.ullTotalPageFile;
        DWORDLONG availVirtual = memInfo.ullAvailPageFile;
        DWORDLONG totalSwap = totalVirtual - totalPhysical;
        DWORDLONG usedSwap = totalSwap - (availVirtual - memInfo.ullAvailPhys);
        if (totalSwap > 0) {
            if (usedSwap < 0) usedSwap = 0;
            double totalGB = (double)totalSwap / (1024 * 1024 * 1024);

            if (usedSwap == 0) {
                snprintf(swap_usage, BUFFERSIZE, "0 B / %.2f GiB (0%%)", totalGB);
            } else {
                double usedGB = (double)usedSwap / (1024 * 1024 * 1024);
                double percentage = 100.0 * usedSwap / totalSwap;

                snprintf(swap_usage, BUFFERSIZE, "%.2f GiB / %.2f GiB (%.0f%%)",
                         usedGB, totalGB, percentage);
            }
        } else {
            strncpy(swap_usage, "Not Available", BUFFERSIZE);
        }
    }
    #else
        FILE *f = fopen("/proc/meminfo", "r");
        if (!f)
            return;

        char buffer[BUFFERSIZE] = { 0 };
        size_t total_kB = 0, free_kB = 0;
        while (fgets(buffer, BUFFERSIZE, f) != NULL) {
            if (!strncmp(buffer, "SwapTotal:", 10))
                sscanf(buffer, "SwapTotal: %zd kB", &total_kB);
            else if (!strncmp(buffer, "SwapFree:", 9))
                sscanf(buffer, "SwapFree: %zd kB", &free_kB);
        }

        if (total_kB == 0) {
            strncpy(swap_usage, "0kB", BUFFERSIZE);
        } else if (total_kB > 0) {
            size_t used_kB = total_kB - free_kB;
            snprintf(swap_usage, BUFFERSIZE,
                "%.2fGB / %.2fGB (%.0f%%)",
                (double)used_kB / 1024 / 1024,
                (double)total_kB / 1024 / 1024,
                (double)used_kB * 100 / total_kB
            );
        }

        fclose(f);
    #endif
    }

void fetch_disk_usage(char *disk_usage) {
    NULL_RETURN(disk_usage);
    strncpy(disk_usage, "Unknown", BUFFERSIZE);

#ifdef _WIN32
    ULARGE_INTEGER freeBytesAvailable, totalBytes, totalFreeBytes;
    if (GetDiskFreeSpaceEx("C:\\", &freeBytesAvailable, &totalBytes, &totalFreeBytes)) {
        ULONGLONG total_bytes = totalBytes.QuadPart;
        ULONGLONG used_bytes = total_bytes - totalFreeBytes.QuadPart;

        snprintf(disk_usage, BUFFERSIZE,
            "%.1fGB / %.1fGB (%.0f%%)",
            (double)used_bytes / 1024 / 1024 / 1024,
            (double)total_bytes / 1024 / 1024 / 1024,
            (double)used_bytes * 100 / total_bytes
        );
    }
#else
    struct statvfs data;
    if (!statvfs("/", &data)) {
        size_t total_bytes = data.f_frsize * data.f_blocks;
        size_t used_bytes = total_bytes - data.f_frsize * data.f_bfree;

        snprintf(disk_usage, BUFFERSIZE,
            "%.1fGB / %.1fGB (%.0f%%)",
            (double)used_bytes / 1024 / 1024 / 1024,
            (double)total_bytes / 1024 / 1024 / 1024,
            (double)used_bytes * 100 / total_bytes
        );
    }
#endif
}

void fetch_process_count(char *process_count) {
    NULL_RETURN(process_count);
    strncpy(process_count, "Unknown", BUFFERSIZE);

#ifdef _WIN32
    DWORD aProcesses[1024], cbNeeded, cProcesses;
    if (!EnumProcesses(aProcesses, sizeof(aProcesses), &cbNeeded)) {
        return;
    }
    cProcesses = cbNeeded / sizeof(DWORD);
    snprintf(process_count, BUFFERSIZE, "%lu", cProcesses);
#else
    char buffer[BUFFERSIZE] = { 0 };

    FILE *f = popen("ps -aux | wc -l", "r");
    if (fgets(buffer, BUFFERSIZE, f) != NULL) {
        strncpy(process_count, buffer, BUFFERSIZE);
    }
    pclose(f);
#endif
}

void fetch_uptime(char *uptime) {
    NULL_RETURN(uptime);
    strncpy(uptime, "Unknown", BUFFERSIZE);

#ifdef _WIN32
    DWORD tickCount = GetTickCount();
    int s = tickCount / 1000;
    int h = s / 3600;
    int m = (s % 3600) / 60;
    s = s % 60;
    snprintf(uptime, BUFFERSIZE, "%02d:%02d:%02d", h, m, s);
#else
    struct sysinfo data;
    if (!sysinfo(&data)) {
        int h = data.uptime / 3600,
            m = data.uptime % 3600 / 60,
            s = data.uptime % 60;
        snprintf(uptime, BUFFERSIZE, "%02d:%02d:%02d", h, m, s);
    }
#endif
}

void fetch_battery_charge(char *battery_charge) {
    NULL_RETURN(battery_charge);
    strncpy(battery_charge, "Unknown", BUFFERSIZE);

#ifdef _WIN32
    SYSTEM_POWER_STATUS powerStatus;
    if (GetSystemPowerStatus(&powerStatus) && powerStatus.BatteryLifePercent <= 100) {
        snprintf(battery_charge, BUFFERSIZE, "%d%%", powerStatus.BatteryLifePercent);
    }
#else
    FILE *f = fopen("/sys/class/power_supply/BAT0/capacity", "r");
    if (!f)
        return;

    char buffer[BUFFERSIZE] = { 0 };
    fgets(buffer, BUFFERSIZE, f);
    strncpy(battery_charge, buffer, BUFFERSIZE);
    size_t length = strlen(battery_charge);
    if (battery_charge[length - 1] == '\n') battery_charge[--length] = '\0';
    if (length < BUFFERSIZE - 1)
        battery_charge[length] = '%';
    fclose(f);
#endif
}

void fetch_stats(system_stats *stats) {
    fetch_user_name(stats->user_name);
    fetch_host_name(stats->host_name);
    fetch_datetime(stats->datetime);
    fetch_os_name(stats->os_name);
    fetch_kernel_version(stats->kernel_version);
    fetch_desktop_name(stats->desktop_name);
    fetch_shell_name(stats->shell_name);
    fetch_terminal_name(stats->terminal_name);
    fetch_cpu_name(stats->cpu_name);
    fetch_cpu_usage(stats->cpu_usage);
    fetch_ram_usage(stats->ram_usage);
    fetch_swap_usage(stats->swap_usage);
    fetch_disk_usage(stats->disk_usage);
    fetch_process_count(stats->process_count);
    fetch_uptime(stats->uptime);
    fetch_battery_charge(stats->battery_charge);
}

void update_dynamic_stats(system_stats *stats) {
    fetch_datetime(stats->datetime);
    fetch_cpu_usage(stats->cpu_usage);
    fetch_ram_usage(stats->ram_usage);
    fetch_swap_usage(stats->swap_usage);
    fetch_process_count(stats->process_count);
    fetch_uptime(stats->uptime);
    fetch_battery_charge(stats->battery_charge);
}

void get_terminal_size(int *columns, int *lines) {
    *columns = 0; *lines = 0;

#ifdef _WIN32
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    if (GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi)) {
        *columns = csbi.srWindow.Right - csbi.srWindow.Left + 1;
        *lines = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;
    }
#else
    char buffer[BUFFERSIZE] = { 0 };
    FILE *f = popen("tput cols", "r");
    if (!f)
        return;
    if (fgets(buffer, BUFFERSIZE, f) != NULL)
        *columns = atoi(buffer);
    pclose(f);

    f = popen("tput lines", "r");
    if (!f)
        return;
    if (fgets(buffer, BUFFERSIZE, f) != NULL)
        *lines = atoi(buffer);
    pclose(f);
#endif
}

void clear_screen(int columns, int lines) {
#ifdef _WIN32
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    COORD coordScreen = { 0, 0 };
    DWORD cCharsWritten;
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    DWORD dwConSize;

    GetConsoleScreenBufferInfo(hConsole, &csbi);
    dwConSize = csbi.dwSize.X * csbi.dwSize.Y;
    FillConsoleOutputCharacter(hConsole, ' ', dwConSize, coordScreen, &cCharsWritten);
    FillConsoleOutputAttribute(hConsole, csbi.wAttributes, dwConSize, coordScreen, &cCharsWritten);
    SetConsoleCursorPosition(hConsole, coordScreen);
#else
    printf(POS COLOR_RESET, 0, 0);
    for (int y = 1; y < lines + 1; y++) {
        printf(POS, y, 0);
        for (int x = 0; x < columns; x++)
            putchar(' ');
    }
#endif
}

void print_logo() {
    //printf(POS "%s", 0, 0, arch_logo_8x15);
    //printf(POS COLOR_CYAN "%s" COLOR_RESET, 0, 0, arch_logo_wide);
    printf(yield_frame(jorb));
}

void draw_line(int length) {
    int x = 0;
    while (x++ < length)
        putchar('-');
}

void print_stats(system_stats stats) {
    int line = 1,
        column = PADDING + 2;
    int namelen = strlen(stats.user_name) + strlen(stats.host_name) + 1;
    printf(POS COLOR_RESET COLOR_CYAN "%*seri-fetch🐯⚙️" COLOR_RESET, line++, column, (namelen - 8) / 2, "");
    printf(POS COLOR_CYAN "%s" COLOR_RESET "@" COLOR_CYAN "%s" COLOR_RESET, line++, column, stats.user_name, stats.host_name);
    printf(POS, line++, column);
    draw_line(namelen);
    printf(POS COLOR_CYAN "Datetime: " COLOR_RESET " %s", line++, column, stats.datetime);
    printf(POS COLOR_CYAN "OS:       " COLOR_RESET " %s", line++, column, stats.os_name);
    printf(POS COLOR_CYAN "Kernel:   " COLOR_RESET " %s", line++, column, stats.kernel_version);
    printf(POS COLOR_CYAN "Desktop:  " COLOR_RESET " %s", line++, column, stats.desktop_name);
    printf(POS COLOR_CYAN "Shell:    " COLOR_RESET " %s", line++, column, stats.shell_name);
    printf(POS COLOR_CYAN "Terminal: " COLOR_RESET " %s", line++, column, stats.terminal_name);
    printf(POS COLOR_CYAN "CPU:      " COLOR_RESET " %s", line++, column, stats.cpu_name);
    printf(POS, line, column);
    for (int i = 0; i < 50; i++) putchar(' '); // bro im lazy ok?
    printf(POS COLOR_CYAN "CPU Usage:" COLOR_RESET " %s", line++, column, stats.cpu_usage);
    printf(POS COLOR_CYAN "Memory:   " COLOR_RESET " %s", line++, column, stats.ram_usage);
    printf(POS COLOR_CYAN "Swap:     " COLOR_RESET " %s", line++, column, stats.swap_usage);
    printf(POS COLOR_CYAN "Disk:     " COLOR_RESET " %s", line++, column, stats.disk_usage);
    printf(POS COLOR_CYAN "Processes:" COLOR_RESET " %s", line++, column, stats.process_count);
    printf(POS COLOR_CYAN "Uptime:   " COLOR_RESET " %s", line++, column, stats.uptime);
    printf(POS COLOR_CYAN "Battery:  " COLOR_RESET " %s", line++, column, stats.battery_charge);
}

#ifdef _WIN32
BOOL WINAPI handle_exit_win(DWORD signal) {
    int columns, lines;
    get_terminal_size(&columns, &lines);
    clear_screen(columns, lines);
    print_stats(sysstats);
    print_logo();

    printf("\n");
    // Show the cursor again
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_CURSOR_INFO cursorInfo;
    GetConsoleCursorInfo(hConsole, &cursorInfo);
    cursorInfo.bVisible = TRUE;
    SetConsoleCursorInfo(hConsole, &cursorInfo);

    WSACleanup();
    exit(0);
    return TRUE;
}
#else
void handle_exit(int /*signal*/) {
    int columns, lines;
    get_terminal_size(&columns, &lines);
    clear_screen(columns, lines);
    print_stats(sysstats);
    print_logo();

    printf("\n");
    system("tput cnorm");
    exit(0);
}
#endif

int main(int argc, char** argv) {
#ifdef _WIN32
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        fprintf(stderr, "WSAStartup failed\n");
        return 1;
    }
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_CURSOR_INFO cursorInfo;
    GetConsoleCursorInfo(hConsole, &cursorInfo);
    cursorInfo.bVisible = FALSE;
    SetConsoleCursorInfo(hConsole, &cursorInfo);

    SetConsoleCtrlHandler(handle_exit_win, TRUE);
#else
    signal(SIGINT, handle_exit);
    system("tput civis");
#endif

    if (argc == 2) {
        if (strcmp(argv[1], "jelly") == 0) {
            jorb = &jelly_jorb;
        } else if (strcmp(argv[1], "erina") == 0) {
            jorb = &erina_jorb;
        } else {
            fprintf(stderr, "Unknown talent: %s\n", argv[1]);
            return 1;
        }
    } else {
        jorb = &jelly_jorb;
    }

    fetch_stats(&sysstats);

    size_t frame = 0;
    int prev_columns = 0, prev_lines = 0;
    int columns, lines;
    while (1) {
        get_terminal_size(&columns, &lines);
        if (prev_columns != columns || prev_lines != lines) {
            clear_screen(columns, lines);
            prev_columns = columns;
            prev_lines = lines;
        }
        print_stats(sysstats);
        print_logo();
        fflush(stdout);
        if (frame % (FPS / 2) == 0) // update stats every 0.5s
            update_dynamic_stats(&sysstats);
        usleep(1000000 / FPS);
        frame++;
    }

#ifdef _WIN32
    handle_exit_win(0);
#else
    handle_exit(0);
#endif

    return 0;
}
