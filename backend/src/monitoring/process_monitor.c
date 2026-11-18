#define __USE_MINGW_ANSI_STDIO 1
#include "process_monitor.h"
#include "cpu_monitor.h"
#include "memory_monitor.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>

#if defined(_WIN32)
#include <windows.h>
#include <tlhelp32.h>
#include <psapi.h>
#else
#include <dirent.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#endif

typedef struct {
    int pid;
    uint64_t ticks;
    double timestamp;
    bool seen;
} pid_cpu_track_t;

static pid_cpu_track_t *s_track_table = NULL;
static int s_track_count = 0;
static int s_track_capacity = 0;

static double get_monotonic_time_sec(void) {
#if defined(_WIN32)
    static LARGE_INTEGER freq;
    static bool freq_init = false;
    if (!freq_init) {
        QueryPerformanceFrequency(&freq);
        freq_init = true;
    }
    LARGE_INTEGER now;
    QueryPerformanceCounter(&now);
    return (double)now.QuadPart / (double)freq.QuadPart;
#else
    struct timespec ts;
    if (clock_gettime(CLOCK_MONOTONIC, &ts) == 0) {
        return (double)ts.tv_sec + (double)ts.tv_nsec / 1e9;
    }
    return (double)time(NULL);
#endif
}

const char *process_state_to_string(char state_char) {
    switch (state_char) {
        case 'R': return "Running";
        case 'S': return "Sleeping";
        case 'D': return "Disk Sleep";
        case 'Z': return "Zombie";
        case 'T': return "Stopped";
        case 't': return "Tracing Stop";
        case 'I': return "Idle";
        case 'X': return "Dead";
        case 'W': return "Paging";
        default:  return "Unknown";
    }
}

uint64_t get_system_uptime_seconds(void) {
    FILE *fp = fopen("/proc/uptime", "r");
    if (fp) {
        double uptime_sec = 0.0;
        if (fscanf(fp, "%lf", &uptime_sec) == 1) {
            fclose(fp);
            return (uint64_t)uptime_sec;
        }
        fclose(fp);
    }
#if defined(_WIN32)
    typedef ULONGLONG (WINAPI *pfnGetTickCount64)(void);
    HMODULE hK = GetModuleHandleA("kernel32.dll");
    if (hK) {
        pfnGetTickCount64 fn = (pfnGetTickCount64)GetProcAddress(hK, "GetTickCount64");
        if (fn) {
            return (uint64_t)(fn() / 1000ULL);
        }
    }
    return (uint64_t)(GetTickCount() / 1000UL);
#else
    return 0;
#endif
}

static double calculate_proc_cpu(int pid, uint64_t current_ticks, double now_sec, int core_count) {
    if (core_count <= 0) core_count = 1;
    double cpu_pct = 0.0;

    int found_idx = -1;
    for (int i = 0; i < s_track_count; i++) {
        if (s_track_table[i].pid == pid) {
            found_idx = i;
            break;
        }
    }

    if (found_idx >= 0) {
        s_track_table[found_idx].seen = true;
        uint64_t prev_ticks = s_track_table[found_idx].ticks;
        double prev_time = s_track_table[found_idx].timestamp;
        double delta_sec = now_sec - prev_time;

        if (delta_sec > 0.05 && current_ticks >= prev_ticks) {
            uint64_t d_ticks = current_ticks - prev_ticks;
#if defined(_WIN32)
            /* On Windows, ticks are in 100-nanosecond units (10,000,000 per sec) */
            double proc_sec = (double)d_ticks / 10000000.0;
            cpu_pct = (proc_sec / delta_sec) * 100.0;
#else
            long hz = sysconf(_SC_CLK_TCK);
            if (hz <= 0) hz = 100;
            double proc_sec = (double)d_ticks / (double)hz;
            cpu_pct = (proc_sec / delta_sec) * 100.0;
#endif
            if (cpu_pct < 0.0) cpu_pct = 0.0;
            if (cpu_pct > 100.0 * core_count) cpu_pct = 100.0 * core_count;
        }

        s_track_table[found_idx].ticks = current_ticks;
        s_track_table[found_idx].timestamp = now_sec;
    } else {
        /* Add new tracking entry */
        if (s_track_count >= s_track_capacity) {
            int new_cap = (s_track_capacity == 0) ? 256 : s_track_capacity * 2;
            pid_cpu_track_t *new_tbl = (pid_cpu_track_t *)realloc(s_track_table, new_cap * sizeof(pid_cpu_track_t));
            if (new_tbl) {
                s_track_table = new_tbl;
                s_track_capacity = new_cap;
            }
        }
        if (s_track_count < s_track_capacity) {
            s_track_table[s_track_count].pid = pid;
            s_track_table[s_track_count].ticks = current_ticks;
            s_track_table[s_track_count].timestamp = now_sec;
            s_track_table[s_track_count].seen = true;
            s_track_count++;
        }
    }

    return cpu_pct;
}

static void prune_unseen_tracking(void) {
    int w = 0;
    for (int r = 0; r < s_track_count; r++) {
        if (s_track_table[r].seen) {
            s_track_table[r].seen = false; /* Reset for next cycle */
            if (w != r) {
                s_track_table[w] = s_track_table[r];
            }
            w++;
        }
    }
    s_track_count = w;
}

static int parse_linux_proc_entry(int pid, process_info_t *proc, uint64_t total_ram_kb,
                                   double now_sec, int core_count) {
    char stat_path[64];
    snprintf(stat_path, sizeof(stat_path), "/proc/%d/stat", pid);

    FILE *fp = fopen(stat_path, "r");
    if (!fp) {
        /* Process may have exited or is inaccessible */
        return -1;
    }

    char buffer[1024];
    if (!fgets(buffer, sizeof(buffer), fp)) {
        fclose(fp);
        return -1;
    }
    fclose(fp);

    /* Locate name between first '(' and last ')' */
    char *open_paren = strchr(buffer, '(');
    char *close_paren = strrchr(buffer, ')');
    if (!open_paren || !close_paren || close_paren < open_paren) {
        return -1;
    }

    proc->pid = pid;

    /* Extract process name */
    size_t name_len = close_paren - open_paren - 1;
    if (name_len >= MAX_PROC_NAME_LEN) name_len = MAX_PROC_NAME_LEN - 1;
    memcpy(proc->name, open_paren + 1, name_len);
    proc->name[name_len] = '\0';

    /* Fields after ')' are: state ppid pgrp session tty_nr tpgid flags minflt ... */
    char *after_paren = close_paren + 2;
    char state_ch = *after_paren;
    proc->state_char = state_ch;
    proc->state_str = process_state_to_string(state_ch);

    int ppid = 0, pgrp = 0, session = 0, tty_nr = 0, tpgid = 0;
    unsigned int flags = 0;
    unsigned long minflt = 0, cminflt = 0, majflt = 0, cmajflt = 0;
    unsigned long utime = 0, stime = 0;
    long cutime = 0, cstime = 0, priority = 0, nice_val = 0, num_threads = 1;
    unsigned long itrealvalue = 0;
    unsigned long long starttime = 0, vsize = 0;
    long rss = 0;

    int parsed = sscanf(after_paren + 2,
        "%d %d %d %d %d %u %lu %lu %lu %lu %lu %lu %ld %ld %ld %ld %ld %lu %llu %llu %ld",
        &ppid, &pgrp, &session, &tty_nr, &tpgid, &flags,
        &minflt, &cminflt, &majflt, &cmajflt,
        &utime, &stime, &cutime, &cstime,
        &priority, &nice_val, &num_threads, &itrealvalue,
        &starttime, &vsize, &rss);

    if (parsed >= 1) proc->ppid = ppid;
    if (parsed >= 15) proc->priority = (int)priority;
    if (parsed >= 16) proc->nice_val = (int)nice_val;
    if (parsed >= 17) proc->num_threads = (int)num_threads;
    if (parsed >= 20) proc->mem_vsz_kb = vsize / 1024;
    
    long page_size_kb = 4;
#if defined(__linux__)
    long ps = sysconf(_SC_PAGESIZE);
    if (ps > 0) page_size_kb = ps / 1024;
#endif
    if (parsed >= 21) {
        proc->mem_rss_kb = (rss > 0) ? (uint64_t)rss * page_size_kb : 0;
    }

    if (total_ram_kb > 0) {
        proc->mem_pct = ((double)proc->mem_rss_kb / (double)total_ram_kb) * 100.0;
    } else {
        proc->mem_pct = 0.0;
    }

    proc->utime_ticks = utime;
    proc->stime_ticks = stime;
    proc->start_time_seconds = starttime;

    uint64_t total_ticks = (uint64_t)utime + (uint64_t)stime;
    proc->cpu_pct = calculate_proc_cpu(pid, total_ticks, now_sec, core_count);

    /* Read /proc/[pid]/cmdline */
    char cmd_path[64];
    snprintf(cmd_path, sizeof(cmd_path), "/proc/%d/cmdline", pid);
    FILE *cmd_fp = fopen(cmd_path, "r");
    if (cmd_fp) {
        size_t bytes = fread(proc->cmdline, 1, sizeof(proc->cmdline) - 1, cmd_fp);
        fclose(cmd_fp);
        if (bytes > 0) {
            proc->cmdline[bytes] = '\0';
            for (size_t i = 0; i < bytes - 1; i++) {
                if (proc->cmdline[i] == '\0') {
                    proc->cmdline[i] = ' ';
                }
            }
        } else {
            snprintf(proc->cmdline, sizeof(proc->cmdline), "[%s]", proc->name);
        }
    } else {
        snprintf(proc->cmdline, sizeof(proc->cmdline), "[%s]", proc->name);
    }

    return 0;
}

#if !defined(_WIN32)
static int scan_linux_proc(process_table_t *out_table, uint64_t total_ram_kb, int core_count) {
    DIR *dir = opendir("/proc");
    if (!dir) return -1;

    double now_sec = get_monotonic_time_sec();
    struct dirent *entry;

    while ((entry = readdir(dir)) != NULL) {
        /* Check if directory name is all digits (PID) */
        char *p = entry->d_name;
        bool is_pid = (*p != '\0');
        while (*p) {
            if (!isdigit((unsigned char)*p)) {
                is_pid = false;
                break;
            }
            p++;
        }
        if (!is_pid) continue;

        int pid = atoi(entry->d_name);
        if (pid <= 0) continue;

        if (out_table->count >= out_table->capacity) {
            int new_cap = (out_table->capacity == 0) ? 128 : out_table->capacity * 2;
            process_info_t *new_procs = (process_info_t *)realloc(out_table->processes,
                                                                 new_cap * sizeof(process_info_t));
            if (!new_procs) {
                closedir(dir);
                return -1;
            }
            out_table->processes = new_procs;
            out_table->capacity = new_cap;
        }

        process_info_t *proc = &out_table->processes[out_table->count];
        memset(proc, 0, sizeof(process_info_t));

        if (parse_linux_proc_entry(pid, proc, total_ram_kb, now_sec, core_count) == 0) {
            out_table->count++;
            out_table->total_processes++;

            switch (proc->state_char) {
                case 'R': out_table->running_count++; break;
                case 'S':
                case 'D':
                case 'I': out_table->sleeping_count++; break;
                case 'T':
                case 't': out_table->stopped_count++; break;
                case 'Z': out_table->zombie_count++; break;
                default:  out_table->idle_count++; break;
            }
        }
    }
    closedir(dir);
    prune_unseen_tracking();
    return 0;
}
#endif

#if defined(_WIN32)
static int scan_windows_processes(process_table_t *out_table, uint64_t total_ram_kb, int core_count) {
    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snap == INVALID_HANDLE_VALUE) return -1;

    PROCESSENTRY32 pe;
    pe.dwSize = sizeof(pe);

    if (!Process32First(snap, &pe)) {
        CloseHandle(snap);
        return -1;
    }

    double now_sec = get_monotonic_time_sec();

    do {
        if (out_table->count >= out_table->capacity) {
            int new_cap = (out_table->capacity == 0) ? 128 : out_table->capacity * 2;
            process_info_t *new_procs = (process_info_t *)realloc(out_table->processes,
                                                                 new_cap * sizeof(process_info_t));
            if (!new_procs) {
                CloseHandle(snap);
                return -1;
            }
            out_table->processes = new_procs;
            out_table->capacity = new_cap;
        }

        process_info_t *proc = &out_table->processes[out_table->count];
        memset(proc, 0, sizeof(process_info_t));

        proc->pid = (int)pe.th32ProcessID;
        proc->ppid = (int)pe.th32ParentProcessID;
        strncpy(proc->name, pe.szExeFile, sizeof(proc->name) - 1);
        proc->name[sizeof(proc->name) - 1] = '\0';
        proc->num_threads = (int)pe.cntThreads;
        proc->priority = (int)pe.pcPriClassBase;
        proc->nice_val = 0;
        proc->state_char = (proc->pid == 0 || proc->pid == 4) ? 'S' : 'R';
        proc->state_str = (proc->state_char == 'R') ? "Running" : "Sleeping";

        /* Memory and CPU via process handle */
        HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pe.th32ProcessID);
        if (!hProcess) {
            hProcess = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pe.th32ProcessID);
        }

        uint64_t total_proc_ticks = 0;
        if (hProcess) {
            PROCESS_MEMORY_COUNTERS pmc;
            if (GetProcessMemoryInfo(hProcess, &pmc, sizeof(pmc))) {
                proc->mem_rss_kb = pmc.WorkingSetSize / 1024;
                proc->mem_vsz_kb = pmc.PagefileUsage / 1024;
            }

            FILETIME createTime, exitTime, kernelTime, userTime;
            if (GetProcessTimes(hProcess, &createTime, &exitTime, &kernelTime, &userTime)) {
                ULARGE_INTEGER u_kern, u_user;
                u_kern.LowPart = kernelTime.dwLowDateTime;
                u_kern.HighPart = kernelTime.dwHighDateTime;
                u_user.LowPart = userTime.dwLowDateTime;
                u_user.HighPart = userTime.dwHighDateTime;

                proc->utime_ticks = u_user.QuadPart;
                proc->stime_ticks = u_kern.QuadPart;
                total_proc_ticks = u_kern.QuadPart + u_user.QuadPart;
            }

            char full_path[MAX_PATH];
            if (GetModuleFileNameExA(hProcess, NULL, full_path, sizeof(full_path))) {
                strncpy(proc->cmdline, full_path, sizeof(proc->cmdline) - 1);
                proc->cmdline[sizeof(proc->cmdline) - 1] = '\0';
            } else {
                snprintf(proc->cmdline, sizeof(proc->cmdline), "[%s]", proc->name);
            }

            CloseHandle(hProcess);
        } else {
            snprintf(proc->cmdline, sizeof(proc->cmdline), "[%s]", proc->name);
        }

        if (total_ram_kb > 0 && proc->mem_rss_kb > 0) {
            proc->mem_pct = ((double)proc->mem_rss_kb / (double)total_ram_kb) * 100.0;
        } else {
            proc->mem_pct = 0.0;
        }

        proc->cpu_pct = calculate_proc_cpu(proc->pid, total_proc_ticks, now_sec, core_count);

        out_table->count++;
        out_table->total_processes++;

        if (proc->cpu_pct > 0.5) {
            out_table->running_count++;
            proc->state_char = 'R';
            proc->state_str = "Running";
        } else {
            out_table->sleeping_count++;
            proc->state_char = 'S';
            proc->state_str = "Sleeping";
        }

    } while (Process32Next(snap, &pe));

    CloseHandle(snap);
    prune_unseen_tracking();
    return 0;
}
#endif

int process_monitor_init(void) {
    if (!s_track_table) {
        s_track_capacity = 256;
        s_track_table = (pid_cpu_track_t *)calloc(s_track_capacity, sizeof(pid_cpu_track_t));
        s_track_count = 0;
    }
    return 0;
}

int process_monitor_update(process_table_t *out_table) {
    if (!out_table) return -1;

    memset(out_table, 0, sizeof(process_table_t));
    out_table->system_uptime_seconds = get_system_uptime_seconds();

    memory_stats_t mem;
    uint64_t total_ram_kb = (memory_monitor_update(&mem) == 0) ? mem.total_kb : 16777216;

    cpu_stats_t cpu;
    int core_count = (cpu_monitor_update(&cpu) == 0 && cpu.core_count > 0) ? cpu.core_count : 4;

#if !defined(_WIN32)
    return scan_linux_proc(out_table, total_ram_kb, core_count);
#else
    return scan_windows_processes(out_table, total_ram_kb, core_count);
#endif
}

int process_get_by_pid(int pid, process_info_t *out_proc) {
    if (!out_proc || pid <= 0) return -1;

    memory_stats_t mem;
    uint64_t total_ram_kb = (memory_monitor_update(&mem) == 0) ? mem.total_kb : 16777216;

    cpu_stats_t cpu;
    int core_count = (cpu_monitor_update(&cpu) == 0 && cpu.core_count > 0) ? cpu.core_count : 4;

    double now_sec = get_monotonic_time_sec();

    /* Try /proc first */
    char stat_path[64];
    snprintf(stat_path, sizeof(stat_path), "/proc/%d/stat", pid);
    FILE *fp = fopen(stat_path, "r");
    if (fp) {
        fclose(fp);
        return parse_linux_proc_entry(pid, out_proc, total_ram_kb, now_sec, core_count);
    }

#if defined(_WIN32)
    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snap == INVALID_HANDLE_VALUE) return -1;

    PROCESSENTRY32 pe;
    pe.dwSize = sizeof(pe);

    if (Process32First(snap, &pe)) {
        do {
            if ((int)pe.th32ProcessID == pid) {
                memset(out_proc, 0, sizeof(process_info_t));
                out_proc->pid = pid;
                out_proc->ppid = (int)pe.th32ParentProcessID;
                strncpy(out_proc->name, pe.szExeFile, sizeof(out_proc->name) - 1);
                out_proc->num_threads = (int)pe.cntThreads;
                out_proc->priority = (int)pe.pcPriClassBase;
                out_proc->state_char = 'S';
                out_proc->state_str = "Sleeping";

                HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pe.th32ProcessID);
                if (!hProcess) {
                    hProcess = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pe.th32ProcessID);
                }

                uint64_t total_proc_ticks = 0;
                if (hProcess) {
                    PROCESS_MEMORY_COUNTERS pmc;
                    if (GetProcessMemoryInfo(hProcess, &pmc, sizeof(pmc))) {
                        out_proc->mem_rss_kb = pmc.WorkingSetSize / 1024;
                        out_proc->mem_vsz_kb = pmc.PagefileUsage / 1024;
                    }

                    FILETIME createTime, exitTime, kernelTime, userTime;
                    if (GetProcessTimes(hProcess, &createTime, &exitTime, &kernelTime, &userTime)) {
                        ULARGE_INTEGER u_kern, u_user;
                        u_kern.LowPart = kernelTime.dwLowDateTime;
                        u_kern.HighPart = kernelTime.dwHighDateTime;
                        u_user.LowPart = userTime.dwLowDateTime;
                        u_user.HighPart = userTime.dwHighDateTime;
                        out_proc->utime_ticks = u_user.QuadPart;
                        out_proc->stime_ticks = u_kern.QuadPart;
                        total_proc_ticks = u_kern.QuadPart + u_user.QuadPart;
                    }

                    char full_path[MAX_PATH];
                    if (GetModuleFileNameExA(hProcess, NULL, full_path, sizeof(full_path))) {
                        strncpy(out_proc->cmdline, full_path, sizeof(out_proc->cmdline) - 1);
                    } else {
                        snprintf(out_proc->cmdline, sizeof(out_proc->cmdline), "[%s]", out_proc->name);
                    }
                    CloseHandle(hProcess);
                } else {
                    snprintf(out_proc->cmdline, sizeof(out_proc->cmdline), "[%s]", out_proc->name);
                }

                if (total_ram_kb > 0 && out_proc->mem_rss_kb > 0) {
                    out_proc->mem_pct = ((double)out_proc->mem_rss_kb / (double)total_ram_kb) * 100.0;
                }

                out_proc->cpu_pct = calculate_proc_cpu(pid, total_proc_ticks, now_sec, core_count);
                if (out_proc->cpu_pct > 0.5) {
                    out_proc->state_char = 'R';
                    out_proc->state_str = "Running";
                }

                CloseHandle(snap);
                return 0;
            }
        } while (Process32Next(snap, &pe));
    }
    CloseHandle(snap);
#endif

    return -1;
}

void process_table_free(process_table_t *table) {
    if (!table) return;
    if (table->processes) {
        free(table->processes);
        table->processes = NULL;
    }
    table->count = 0;
    table->capacity = 0;
    table->total_processes = 0;
}

void process_monitor_cleanup(void) {
    if (s_track_table) {
        free(s_track_table);
        s_track_table = NULL;
        s_track_count = 0;
        s_track_capacity = 0;
    }
}
