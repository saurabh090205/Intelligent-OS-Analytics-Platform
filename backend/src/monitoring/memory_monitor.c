#define __USE_MINGW_ANSI_STDIO 1
#include "memory_monitor.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(_WIN32)
#include <windows.h>
#endif

static double s_last_mem_util = 0.0;

static int parse_proc_meminfo(memory_stats_t *out_stats) {
    FILE *fp = fopen("/proc/meminfo", "r");
    if (!fp) return -1;

    char line[256];
    uint64_t total = 0, free_mem = 0, available = 0;
    uint64_t buffers = 0, cached = 0;
    uint64_t swap_total = 0, swap_free = 0;
    int found_available = 0;

    while (fgets(line, sizeof(line), fp)) {
        char key[64];
        unsigned long long val = 0;
        if (sscanf(line, "%63[^:]: %llu", key, &val) == 2) {
            if (strcmp(key, "MemTotal") == 0) total = val;
            else if (strcmp(key, "MemFree") == 0) free_mem = val;
            else if (strcmp(key, "MemAvailable") == 0) { available = val; found_available = 1; }
            else if (strcmp(key, "Buffers") == 0) buffers = val;
            else if (strcmp(key, "Cached") == 0) cached = val;
            else if (strcmp(key, "SwapTotal") == 0) swap_total = val;
            else if (strcmp(key, "SwapFree") == 0) swap_free = val;
        }
    }
    fclose(fp);

    if (total == 0) return -1;

    if (!found_available) {
        available = free_mem + buffers + cached;
    }

    uint64_t used = (total > available) ? (total - available) : 0;
    double util = ((double)used / (double)total) * 100.0;
    if (util < 0.0) util = 0.0;
    if (util > 100.0) util = 100.0;

    out_stats->total_kb = total;
    out_stats->free_kb = free_mem;
    out_stats->available_kb = available;
    out_stats->used_kb = used;
    out_stats->utilization_pct = util;
    out_stats->buffers_kb = buffers;
    out_stats->cached_kb = cached;
    out_stats->swap_total_kb = swap_total;
    out_stats->swap_free_kb = swap_free;
    out_stats->swap_used_kb = (swap_total > swap_free) ? (swap_total - swap_free) : 0;

    s_last_mem_util = util;
    return 0;
}

#if defined(_WIN32)
static int read_windows_memory(memory_stats_t *out_stats) {
    MEMORYSTATUSEX statex;
    statex.dwLength = sizeof(statex);
    if (!GlobalMemoryStatusEx(&statex)) {
        return -1;
    }

    uint64_t total_kb = statex.ullTotalPhys / 1024;
    uint64_t avail_kb = statex.ullAvailPhys / 1024;
    uint64_t used_kb = (total_kb > avail_kb) ? (total_kb - avail_kb) : 0;

    uint64_t page_total_kb = statex.ullTotalPageFile / 1024;
    uint64_t page_avail_kb = statex.ullAvailPageFile / 1024;

    double util = (total_kb > 0) ? (((double)used_kb / (double)total_kb) * 100.0) : (double)statex.dwMemoryLoad;
    if (util < 0.0) util = 0.0;
    if (util > 100.0) util = 100.0;

    out_stats->total_kb = total_kb;
    out_stats->free_kb = avail_kb;
    out_stats->available_kb = avail_kb;
    out_stats->used_kb = used_kb;
    out_stats->utilization_pct = util;
    out_stats->buffers_kb = 0;
    out_stats->cached_kb = 0;
    out_stats->swap_total_kb = (page_total_kb > total_kb) ? (page_total_kb - total_kb) : 0;
    out_stats->swap_free_kb = (page_avail_kb > avail_kb) ? (page_avail_kb - avail_kb) : 0;
    out_stats->swap_used_kb = (out_stats->swap_total_kb > out_stats->swap_free_kb) ?
                              (out_stats->swap_total_kb - out_stats->swap_free_kb) : 0;

    s_last_mem_util = util;
    return 0;
}
#endif

int memory_monitor_init(void) {
    s_last_mem_util = 0.0;
    return 0;
}

int memory_monitor_update(memory_stats_t *out_stats) {
    if (!out_stats) return -1;

    /* Try reading Linux /proc/meminfo first */
    if (parse_proc_meminfo(out_stats) == 0) {
        return 0;
    }

#if defined(_WIN32)
    if (read_windows_memory(out_stats) == 0) {
        return 0;
    }
#endif

    return -1;
}

double memory_get_current_utilization(void) {
    return s_last_mem_util;
}

void memory_monitor_cleanup(void) {
}
