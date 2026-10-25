#ifndef MEMORY_MONITOR_H
#define MEMORY_MONITOR_H

#include <stdint.h>

typedef struct {
    uint64_t total_kb;
    uint64_t free_kb;
    uint64_t available_kb;
    uint64_t used_kb;
    double utilization_pct;
    uint64_t buffers_kb;
    uint64_t cached_kb;
    uint64_t swap_total_kb;
    uint64_t swap_free_kb;
    uint64_t swap_used_kb;
} memory_stats_t;

int memory_monitor_init(void);
int memory_monitor_update(memory_stats_t *out_stats);
double memory_get_current_utilization(void);
void memory_monitor_cleanup(void);

#endif /* MEMORY_MONITOR_H */
