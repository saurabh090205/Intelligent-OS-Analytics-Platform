#define __USE_MINGW_ANSI_STDIO 1
#include "cpu_monitor.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(_WIN32)
#include <windows.h>
#else
#include <unistd.h>
#endif

static uint64_t s_prev_total = 0;
static uint64_t s_prev_idle = 0;
static double s_last_utilization = 0.0;
static cpu_stats_t s_cached_stats;
static bool s_initialized = false;

#if defined(_WIN32)
static uint64_t filetime_to_uint64(const FILETIME *ft) {
    ULARGE_INTEGER uli;
    uli.LowPart = ft->dwLowDateTime;
    uli.HighPart = ft->dwHighDateTime;
    return uli.QuadPart;
}

static uint64_t s_win_prev_idle = 0;
static uint64_t s_win_prev_kernel = 0;
static uint64_t s_win_prev_user = 0;
#endif

static int read_cpuinfo_linux(char *model_out, size_t model_sz, int *core_count_out) {
    FILE *fp = fopen("/proc/cpuinfo", "r");
    if (!fp) return -1;

    char line[256];
    int count = 0;
    model_out[0] = '\0';

    while (fgets(line, sizeof(line), fp)) {
        if (strncmp(line, "model name", 10) == 0 && model_out[0] == '\0') {
            char *colon = strchr(line, ':');
            if (colon) {
                colon++;
                while (*colon == ' ' || *colon == '\t') colon++;
                strncpy(model_out, colon, model_sz - 1);
                model_out[model_sz - 1] = '\0';
                /* Remove trailing newline */
                size_t l = strlen(model_out);
                while (l > 0 && (model_out[l - 1] == '\n' || model_out[l - 1] == '\r')) {
                    model_out[--l] = '\0';
                }
            }
        } else if (strncmp(line, "processor", 9) == 0) {
            count++;
        }
    }
    fclose(fp);

    if (count > 0 && core_count_out) {
        *core_count_out = count;
    }
    return 0;
}

static int parse_proc_stat(cpu_stats_t *out_stats) {
    FILE *fp = fopen("/proc/stat", "r");
    if (!fp) return -1;

    char line[512];
    int core_idx = 0;

    while (fgets(line, sizeof(line), fp)) {
        if (strncmp(line, "cpu ", 4) == 0) {
            unsigned long long user = 0, nice = 0, system = 0, idle = 0;
            unsigned long long iowait = 0, irq = 0, softirq = 0, steal = 0;
            
            sscanf(line + 4, "%llu %llu %llu %llu %llu %llu %llu %llu",
                   &user, &nice, &system, &idle, &iowait, &irq, &softirq, &steal);
            
            uint64_t cur_idle = idle + iowait;
            uint64_t cur_total = user + nice + system + idle + iowait + irq + softirq + steal;
            
            out_stats->total_jiffies = cur_total;
            out_stats->idle_jiffies = cur_idle;

            if (s_prev_total > 0 && cur_total > s_prev_total) {
                uint64_t delta_total = cur_total - s_prev_total;
                uint64_t delta_idle = (cur_idle >= s_prev_idle) ? (cur_idle - s_prev_idle) : 0;
                if (delta_total > 0 && delta_total >= delta_idle) {
                    double util = (1.0 - ((double)delta_idle / (double)delta_total)) * 100.0;
                    if (util < 0.0) util = 0.0;
                    if (util > 100.0) util = 100.0;
                    out_stats->overall_utilization = util;
                } else {
                    out_stats->overall_utilization = 0.0;
                }
            } else {
                out_stats->overall_utilization = s_last_utilization;
            }

            s_prev_total = cur_total;
            s_prev_idle = cur_idle;
            s_last_utilization = out_stats->overall_utilization;

        } else if (strncmp(line, "cpu", 3) == 0 && line[3] >= '0' && line[3] <= '9') {
            if (core_idx < MAX_CPU_CORES) {
                int c_id = 0;
                unsigned long long user = 0, nice = 0, system = 0, idle = 0;
                unsigned long long iowait = 0, irq = 0, softirq = 0, steal = 0;
                
                sscanf(line, "cpu%d %llu %llu %llu %llu %llu %llu %llu %llu",
                       &c_id, &user, &nice, &system, &idle, &iowait, &irq, &softirq, &steal);
                
                uint64_t cur_idle = idle + iowait;
                uint64_t cur_total = user + nice + system + idle + iowait + irq + softirq + steal;
                
                out_stats->cores[core_idx].core_id = c_id;
                if (out_stats->cores[core_idx].prev_total > 0 && cur_total > out_stats->cores[core_idx].prev_total) {
                    uint64_t d_tot = cur_total - out_stats->cores[core_idx].prev_total;
                    uint64_t d_idl = (cur_idle >= out_stats->cores[core_idx].prev_idle) ? 
                                     (cur_idle - out_stats->cores[core_idx].prev_idle) : 0;
                    double c_util = (d_tot > 0) ? (1.0 - ((double)d_idl / (double)d_tot)) * 100.0 : 0.0;
                    if (c_util < 0.0) c_util = 0.0;
                    if (c_util > 100.0) c_util = 100.0;
                    out_stats->cores[core_idx].utilization_pct = c_util;
                } else {
                    out_stats->cores[core_idx].utilization_pct = out_stats->overall_utilization;
                }
                out_stats->cores[core_idx].prev_total = cur_total;
                out_stats->cores[core_idx].prev_idle = cur_idle;
                core_idx++;
            }
        }
    }
    fclose(fp);

    out_stats->active_cores = core_idx;
    if (out_stats->core_count <= 0) {
        out_stats->core_count = core_idx > 0 ? core_idx : 1;
    }

    /* Read cpuinfo for model name if empty */
    if (out_stats->model_name[0] == '\0') {
        int count_from_cpuinfo = 0;
        read_cpuinfo_linux(out_stats->model_name, sizeof(out_stats->model_name), &count_from_cpuinfo);
        if (count_from_cpuinfo > 0 && out_stats->core_count <= 1) {
            out_stats->core_count = count_from_cpuinfo;
        }
    }

#if defined(__linux__)
    long hz = sysconf(_SC_CLK_TCK);
    out_stats->user_hz = (hz > 0) ? (int)hz : 100;
#else
    out_stats->user_hz = 100;
#endif

    return 0;
}

#if defined(_WIN32)
typedef BOOL (WINAPI *pfnGetSystemTimes)(LPFILETIME, LPFILETIME, LPFILETIME);

static int read_windows_cpu(cpu_stats_t *out_stats) {
    FILETIME idleTime, kernelTime, userTime;
    HMODULE hK = GetModuleHandleA("kernel32.dll");
    if (!hK) return -1;
    pfnGetSystemTimes fnGetTimes = (pfnGetSystemTimes)GetProcAddress(hK, "GetSystemTimes");
    if (!fnGetTimes || !fnGetTimes(&idleTime, &kernelTime, &userTime)) {
        return -1;
    }

    uint64_t cur_idle = filetime_to_uint64(&idleTime);
    uint64_t cur_kernel = filetime_to_uint64(&kernelTime);
    uint64_t cur_user = filetime_to_uint64(&userTime);

    SYSTEM_INFO sysInfo;
    GetSystemInfo(&sysInfo);
    out_stats->core_count = (int)sysInfo.dwNumberOfProcessors;
    out_stats->active_cores = out_stats->core_count;
    if (out_stats->active_cores > MAX_CPU_CORES) {
        out_stats->active_cores = MAX_CPU_CORES;
    }

    if (out_stats->model_name[0] == '\0') {
        snprintf(out_stats->model_name, sizeof(out_stats->model_name),
                 "Windows Host x86_64 (%d Logical Processors)", out_stats->core_count);
    }
    out_stats->user_hz = 100;

    out_stats->total_jiffies = (cur_kernel + cur_user);
    out_stats->idle_jiffies = cur_idle;

    if (s_win_prev_kernel > 0 || s_win_prev_user > 0) {
        uint64_t d_kernel = (cur_kernel >= s_win_prev_kernel) ? (cur_kernel - s_win_prev_kernel) : 0;
        uint64_t d_user = (cur_user >= s_win_prev_user) ? (cur_user - s_win_prev_user) : 0;
        uint64_t d_idle = (cur_idle >= s_win_prev_idle) ? (cur_idle - s_win_prev_idle) : 0;

        uint64_t sys_total = d_kernel + d_user;
        if (sys_total > 0 && sys_total >= d_idle) {
            double util = (1.0 - ((double)d_idle / (double)sys_total)) * 100.0;
            if (util < 0.0) util = 0.0;
            if (util > 100.0) util = 100.0;
            out_stats->overall_utilization = util;
        } else {
            out_stats->overall_utilization = s_last_utilization;
        }
    } else {
        out_stats->overall_utilization = 0.0;
    }

    s_win_prev_idle = cur_idle;
    s_win_prev_kernel = cur_kernel;
    s_win_prev_user = cur_user;
    s_last_utilization = out_stats->overall_utilization;

    /* Distribute approx per-core representation */
    for (int i = 0; i < out_stats->active_cores; i++) {
        out_stats->cores[i].core_id = i;
        out_stats->cores[i].utilization_pct = out_stats->overall_utilization;
    }

    return 0;
}
#endif

int cpu_monitor_init(void) {
    memset(&s_cached_stats, 0, sizeof(s_cached_stats));
    s_prev_total = 0;
    s_prev_idle = 0;
    s_last_utilization = 0.0;

    /* Prime initial sample */
    cpu_monitor_update(&s_cached_stats);
    s_initialized = true;
    return 0;
}

int cpu_monitor_update(cpu_stats_t *out_stats) {
    if (!out_stats) return -1;

    /* Try reading Linux /proc/stat first */
    if (parse_proc_stat(out_stats) == 0) {
        memcpy(&s_cached_stats, out_stats, sizeof(cpu_stats_t));
        return 0;
    }

#if defined(_WIN32)
    /* Fallback on Windows if /proc is not available */
    if (read_windows_cpu(out_stats) == 0) {
        memcpy(&s_cached_stats, out_stats, sizeof(cpu_stats_t));
        return 0;
    }
#endif

    return -1;
}

double cpu_get_current_utilization(void) {
    return s_last_utilization;
}

void cpu_monitor_cleanup(void) {
    s_initialized = false;
}
