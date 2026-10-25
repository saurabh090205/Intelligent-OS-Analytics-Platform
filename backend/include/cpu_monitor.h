#ifndef CPU_MONITOR_H
#define CPU_MONITOR_H

#include <stdint.h>
#include <stdbool.h>

#define MAX_CPU_CORES 128

typedef struct {
    int core_id;
    double utilization_pct;
    uint64_t prev_total;
    uint64_t prev_idle;
} cpu_core_info_t;

typedef struct {
    double overall_utilization;
    int core_count;
    int user_hz;
    uint64_t total_jiffies;
    uint64_t idle_jiffies;
    char model_name[128];
    cpu_core_info_t cores[MAX_CPU_CORES];
    int active_cores;
} cpu_stats_t;

int cpu_monitor_init(void);
int cpu_monitor_update(cpu_stats_t *out_stats);
double cpu_get_current_utilization(void);
void cpu_monitor_cleanup(void);

#endif /* CPU_MONITOR_H */
