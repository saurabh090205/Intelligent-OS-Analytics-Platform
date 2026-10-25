#ifndef CPU_SCHEDULER_H
#define CPU_SCHEDULER_H

#include <stdint.h>
#include <stdbool.h>

#define MAX_SCHEDULE_PROCESSES 128
#define MAX_GANTT_SEGMENTS 1024

typedef enum {
    SCHED_ALGO_FCFS = 0,
    SCHED_ALGO_SJF,
    SCHED_ALGO_SRTF,
    SCHED_ALGO_PRIORITY,
    SCHED_ALGO_ROUND_ROBIN,
    SCHED_ALGO_COUNT
} sched_algo_t;

typedef struct {
    int pid;
    char name[32];
    int arrival_time;
    int burst_time;
    int priority;          /* Lower number = higher priority */
    int remaining_time;
    int start_time;        /* First time scheduled */
    int completion_time;
    int turnaround_time;   /* CT - AT */
    int waiting_time;      /* TAT - BT */
    int response_time;     /* First Start - AT */
    bool has_started;
    bool is_completed;
} sched_process_t;

typedef struct {
    int pid;
    char name[32];
    int start_time;
    int end_time;
    bool is_idle;
} gantt_segment_t;

typedef struct {
    sched_algo_t algo;
    const char *algo_name;
    sched_process_t processes[MAX_SCHEDULE_PROCESSES];
    int process_count;
    gantt_segment_t gantt[MAX_GANTT_SEGMENTS];
    int gantt_count;
    double avg_waiting_time;
    double avg_turnaround_time;
    double avg_response_time;
    double cpu_utilization_pct;
    double throughput;     /* processes per time unit */
    int total_time;
    int idle_time;
} sched_result_t;

typedef struct {
    sched_result_t results[SCHED_ALGO_COUNT];
    sched_algo_t recommended_algo;
    const char *recommended_name;
    char recommendation_reason[512];
    double variance_burst;
    bool has_staggered_arrivals;
} sched_comparison_t;

int run_fcfs(const sched_process_t *inputs, int count, sched_result_t *out_result);
int run_sjf(const sched_process_t *inputs, int count, sched_result_t *out_result);
int run_srtf(const sched_process_t *inputs, int count, sched_result_t *out_result);
int run_priority(const sched_process_t *inputs, int count, bool preemptive, sched_result_t *out_result);
int run_round_robin(const sched_process_t *inputs, int count, int quantum, sched_result_t *out_result);

int run_all_and_compare(const sched_process_t *inputs, int count, int quantum,
                        sched_comparison_t *out_comp);

const char *sched_algo_to_string(sched_algo_t algo);

#endif /* CPU_SCHEDULER_H */
