#ifndef PROCESS_MONITOR_H
#define PROCESS_MONITOR_H

#include <stdint.h>
#include <stdbool.h>

#define MAX_PROC_NAME_LEN 128
#define MAX_CMDLINE_LEN 512

typedef struct {
    int pid;
    int ppid;
    char name[MAX_PROC_NAME_LEN];
    char state_char;
    const char *state_str;
    double cpu_pct;
    uint64_t mem_rss_kb;
    uint64_t mem_vsz_kb;
    double mem_pct;
    int num_threads;
    int priority;
    int nice_val;
    char cmdline[MAX_CMDLINE_LEN];
    uint64_t utime_ticks;
    uint64_t stime_ticks;
    uint64_t start_time_seconds;
} process_info_t;

typedef struct {
    process_info_t *processes;
    int count;
    int capacity;
    int total_processes;
    int running_count;
    int sleeping_count;
    int stopped_count;
    int zombie_count;
    int idle_count;
    uint64_t system_uptime_seconds;
} process_table_t;

int process_monitor_init(void);
int process_monitor_update(process_table_t *out_table);
void process_table_free(process_table_t *table);
int process_get_by_pid(int pid, process_info_t *out_proc);
uint64_t get_system_uptime_seconds(void);
void process_monitor_cleanup(void);

const char *process_state_to_string(char state_char);

#endif /* PROCESS_MONITOR_H */
