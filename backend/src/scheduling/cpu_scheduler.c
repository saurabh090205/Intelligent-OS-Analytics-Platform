#define __USE_MINGW_ANSI_STDIO 1
#include "cpu_scheduler.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

const char *sched_algo_to_string(sched_algo_t algo) {
    switch (algo) {
        case SCHED_ALGO_FCFS: return "First Come First Serve (FCFS)";
        case SCHED_ALGO_SJF: return "Shortest Job First (SJF)";
        case SCHED_ALGO_SRTF: return "Shortest Remaining Time First (SRTF)";
        case SCHED_ALGO_PRIORITY: return "Priority Scheduling";
        case SCHED_ALGO_ROUND_ROBIN: return "Round Robin (RR)";
        default: return "Unknown";
    }
}

static void add_gantt_segment(sched_result_t *res, int pid, const char *name, int start, int end, bool is_idle) {
    if (start >= end) return;
    if (res->gantt_count > 0) {
        gantt_segment_t *prev = &res->gantt[res->gantt_count - 1];
        if (prev->pid == pid && prev->is_idle == is_idle && prev->end_time == start) {
            prev->end_time = end;
            return;
        }
    }
    if (res->gantt_count < MAX_GANTT_SEGMENTS) {
        gantt_segment_t *seg = &res->gantt[res->gantt_count++];
        seg->pid = pid;
        seg->start_time = start;
        seg->end_time = end;
        seg->is_idle = is_idle;
        strncpy(seg->name, name ? name : "IDLE", sizeof(seg->name) - 1);
        seg->name[sizeof(seg->name) - 1] = '\0';
    }
}

static void calculate_averages(sched_result_t *res, int total_burst) {
    if (res->process_count <= 0) {
        res->avg_waiting_time = 0.0;
        res->avg_turnaround_time = 0.0;
        res->avg_response_time = 0.0;
        res->cpu_utilization_pct = 0.0;
        res->throughput = 0.0;
        return;
    }

    double sum_wt = 0.0, sum_tat = 0.0, sum_rt = 0.0;
    for (int i = 0; i < res->process_count; i++) {
        sum_wt += res->processes[i].waiting_time;
        sum_tat += res->processes[i].turnaround_time;
        sum_rt += res->processes[i].response_time;
    }

    res->avg_waiting_time = sum_wt / res->process_count;
    res->avg_turnaround_time = sum_tat / res->process_count;
    res->avg_response_time = sum_rt / res->process_count;

    if (res->total_time > 0) {
        res->cpu_utilization_pct = ((double)total_burst / res->total_time) * 100.0;
        res->throughput = (double)res->process_count / res->total_time;
    } else {
        res->cpu_utilization_pct = 0.0;
        res->throughput = 0.0;
    }
}

/* 1. First Come First Serve (FCFS) */
int run_fcfs(const sched_process_t *inputs, int count, sched_result_t *out_result) {
    if (!inputs || count <= 0 || !out_result) return -1;
    memset(out_result, 0, sizeof(sched_result_t));
    out_result->algo = SCHED_ALGO_FCFS;
    out_result->algo_name = sched_algo_to_string(SCHED_ALGO_FCFS);
    out_result->process_count = count > MAX_SCHEDULE_PROCESSES ? MAX_SCHEDULE_PROCESSES : count;

    for (int i = 0; i < out_result->process_count; i++) {
        out_result->processes[i] = inputs[i];
        out_result->processes[i].remaining_time = inputs[i].burst_time;
        out_result->processes[i].has_started = false;
        out_result->processes[i].is_completed = false;
    }

    int current_time = 0;
    int completed = 0;
    int total_burst = 0;

    while (completed < out_result->process_count) {
        int best_idx = -1;
        int min_arrival = 1000000000;

        for (int i = 0; i < out_result->process_count; i++) {
            if (!out_result->processes[i].is_completed) {
                if (out_result->processes[i].arrival_time < min_arrival) {
                    min_arrival = out_result->processes[i].arrival_time;
                    best_idx = i;
                }
            }
        }

        if (best_idx == -1) break;

        sched_process_t *p = &out_result->processes[best_idx];

        if (current_time < p->arrival_time) {
            add_gantt_segment(out_result, -1, "IDLE", current_time, p->arrival_time, true);
            out_result->idle_time += (p->arrival_time - current_time);
            current_time = p->arrival_time;
        }

        p->start_time = current_time;
        p->has_started = true;
        p->response_time = p->start_time - p->arrival_time;

        int finish_time = current_time + p->burst_time;
        add_gantt_segment(out_result, p->pid, p->name, current_time, finish_time, false);

        current_time = finish_time;
        p->completion_time = current_time;
        p->turnaround_time = p->completion_time - p->arrival_time;
        p->waiting_time = p->turnaround_time - p->burst_time;
        p->is_completed = true;
        total_burst += p->burst_time;
        completed++;
    }

    out_result->total_time = current_time;
    calculate_averages(out_result, total_burst);
    return 0;
}

/* 2. Shortest Job First (SJF - Non-Preemptive) */
int run_sjf(const sched_process_t *inputs, int count, sched_result_t *out_result) {
    if (!inputs || count <= 0 || !out_result) return -1;
    memset(out_result, 0, sizeof(sched_result_t));
    out_result->algo = SCHED_ALGO_SJF;
    out_result->algo_name = sched_algo_to_string(SCHED_ALGO_SJF);
    out_result->process_count = count > MAX_SCHEDULE_PROCESSES ? MAX_SCHEDULE_PROCESSES : count;

    for (int i = 0; i < out_result->process_count; i++) {
        out_result->processes[i] = inputs[i];
        out_result->processes[i].remaining_time = inputs[i].burst_time;
        out_result->processes[i].has_started = false;
        out_result->processes[i].is_completed = false;
    }

    int current_time = 0;
    int completed = 0;
    int total_burst = 0;

    while (completed < out_result->process_count) {
        int best_idx = -1;
        int min_burst = 1000000000;

        for (int i = 0; i < out_result->process_count; i++) {
            sched_process_t *p = &out_result->processes[i];
            if (!p->is_completed && p->arrival_time <= current_time) {
                if (p->burst_time < min_burst) {
                    min_burst = p->burst_time;
                    best_idx = i;
                } else if (p->burst_time == min_burst) {
                    if (best_idx == -1 || p->arrival_time < out_result->processes[best_idx].arrival_time) {
                        best_idx = i;
                    }
                }
            }
        }

        if (best_idx == -1) {
            /* No arrived process yet; jump to earliest arrival */
            int next_arrival = 1000000000;
            for (int i = 0; i < out_result->process_count; i++) {
                if (!out_result->processes[i].is_completed) {
                    if (out_result->processes[i].arrival_time < next_arrival) {
                        next_arrival = out_result->processes[i].arrival_time;
                    }
                }
            }
            if (next_arrival > current_time && next_arrival < 1000000000) {
                add_gantt_segment(out_result, -1, "IDLE", current_time, next_arrival, true);
                out_result->idle_time += (next_arrival - current_time);
                current_time = next_arrival;
            } else {
                break;
            }
            continue;
        }

        sched_process_t *p = &out_result->processes[best_idx];
        p->start_time = current_time;
        p->has_started = true;
        p->response_time = p->start_time - p->arrival_time;

        int finish_time = current_time + p->burst_time;
        add_gantt_segment(out_result, p->pid, p->name, current_time, finish_time, false);

        current_time = finish_time;
        p->completion_time = current_time;
        p->turnaround_time = p->completion_time - p->arrival_time;
        p->waiting_time = p->turnaround_time - p->burst_time;
        p->is_completed = true;
        total_burst += p->burst_time;
        completed++;
    }

    out_result->total_time = current_time;
    calculate_averages(out_result, total_burst);
    return 0;
}

/* 3. Shortest Remaining Time First (SRTF - Preemptive SJF) */
int run_srtf(const sched_process_t *inputs, int count, sched_result_t *out_result) {
    if (!inputs || count <= 0 || !out_result) return -1;
    memset(out_result, 0, sizeof(sched_result_t));
    out_result->algo = SCHED_ALGO_SRTF;
    out_result->algo_name = sched_algo_to_string(SCHED_ALGO_SRTF);
    out_result->process_count = count > MAX_SCHEDULE_PROCESSES ? MAX_SCHEDULE_PROCESSES : count;

    int total_burst = 0;
    for (int i = 0; i < out_result->process_count; i++) {
        out_result->processes[i] = inputs[i];
        out_result->processes[i].remaining_time = inputs[i].burst_time;
        out_result->processes[i].has_started = false;
        out_result->processes[i].is_completed = false;
        total_burst += inputs[i].burst_time;
    }

    int current_time = 0;
    int completed = 0;
    int current_running_idx = -1;
    int seg_start_time = 0;

    while (completed < out_result->process_count) {
        int best_idx = -1;
        int min_rem = 1000000000;

        for (int i = 0; i < out_result->process_count; i++) {
            sched_process_t *p = &out_result->processes[i];
            if (!p->is_completed && p->arrival_time <= current_time) {
                if (p->remaining_time < min_rem) {
                    min_rem = p->remaining_time;
                    best_idx = i;
                } else if (p->remaining_time == min_rem) {
                    if (best_idx == -1 || p->arrival_time < out_result->processes[best_idx].arrival_time) {
                        best_idx = i;
                    }
                }
            }
        }

        if (best_idx == -1) {
            if (current_running_idx != -1) {
                sched_process_t *prev_p = &out_result->processes[current_running_idx];
                add_gantt_segment(out_result, prev_p->pid, prev_p->name, seg_start_time, current_time, false);
                current_running_idx = -1;
            }
            int next_arr = 1000000000;
            for (int i = 0; i < out_result->process_count; i++) {
                if (!out_result->processes[i].is_completed && out_result->processes[i].arrival_time < next_arr) {
                    next_arr = out_result->processes[i].arrival_time;
                }
            }
            if (next_arr > current_time && next_arr < 1000000000) {
                add_gantt_segment(out_result, -1, "IDLE", current_time, next_arr, true);
                out_result->idle_time += (next_arr - current_time);
                current_time = next_arr;
            } else {
                break;
            }
            continue;
        }

        if (best_idx != current_running_idx) {
            if (current_running_idx != -1) {
                sched_process_t *prev_p = &out_result->processes[current_running_idx];
                add_gantt_segment(out_result, prev_p->pid, prev_p->name, seg_start_time, current_time, false);
            }
            current_running_idx = best_idx;
            seg_start_time = current_time;
        }

        sched_process_t *p = &out_result->processes[best_idx];
        if (!p->has_started) {
            p->start_time = current_time;
            p->response_time = p->start_time - p->arrival_time;
            p->has_started = true;
        }

        /* Advance by 1 unit */
        p->remaining_time--;
        current_time++;

        if (p->remaining_time == 0) {
            p->completion_time = current_time;
            p->turnaround_time = p->completion_time - p->arrival_time;
            p->waiting_time = p->turnaround_time - p->burst_time;
            p->is_completed = true;
            completed++;

            add_gantt_segment(out_result, p->pid, p->name, seg_start_time, current_time, false);
            current_running_idx = -1;
        }
    }

    if (current_running_idx != -1) {
        sched_process_t *prev_p = &out_result->processes[current_running_idx];
        add_gantt_segment(out_result, prev_p->pid, prev_p->name, seg_start_time, current_time, false);
    }

    out_result->total_time = current_time;
    calculate_averages(out_result, total_burst);
    return 0;
}

/* 4. Priority Scheduling (Non-Preemptive, Lower Value = Higher Priority) */
int run_priority(const sched_process_t *inputs, int count, bool preemptive, sched_result_t *out_result) {
    (void)preemptive; /* Standard non-preemptive priority with tie-breaking */
    if (!inputs || count <= 0 || !out_result) return -1;
    memset(out_result, 0, sizeof(sched_result_t));
    out_result->algo = SCHED_ALGO_PRIORITY;
    out_result->algo_name = sched_algo_to_string(SCHED_ALGO_PRIORITY);
    out_result->process_count = count > MAX_SCHEDULE_PROCESSES ? MAX_SCHEDULE_PROCESSES : count;

    for (int i = 0; i < out_result->process_count; i++) {
        out_result->processes[i] = inputs[i];
        out_result->processes[i].remaining_time = inputs[i].burst_time;
        out_result->processes[i].has_started = false;
        out_result->processes[i].is_completed = false;
    }

    int current_time = 0;
    int completed = 0;
    int total_burst = 0;

    while (completed < out_result->process_count) {
        int best_idx = -1;
        int highest_pri = 1000000000;

        for (int i = 0; i < out_result->process_count; i++) {
            sched_process_t *p = &out_result->processes[i];
            if (!p->is_completed && p->arrival_time <= current_time) {
                if (p->priority < highest_pri) {
                    highest_pri = p->priority;
                    best_idx = i;
                } else if (p->priority == highest_pri) {
                    if (best_idx == -1 || p->arrival_time < out_result->processes[best_idx].arrival_time) {
                        best_idx = i;
                    }
                }
            }
        }

        if (best_idx == -1) {
            int next_arr = 1000000000;
            for (int i = 0; i < out_result->process_count; i++) {
                if (!out_result->processes[i].is_completed && out_result->processes[i].arrival_time < next_arr) {
                    next_arr = out_result->processes[i].arrival_time;
                }
            }
            if (next_arr > current_time && next_arr < 1000000000) {
                add_gantt_segment(out_result, -1, "IDLE", current_time, next_arr, true);
                out_result->idle_time += (next_arr - current_time);
                current_time = next_arr;
            } else {
                break;
            }
            continue;
        }

        sched_process_t *p = &out_result->processes[best_idx];
        p->start_time = current_time;
        p->has_started = true;
        p->response_time = p->start_time - p->arrival_time;

        int finish_time = current_time + p->burst_time;
        add_gantt_segment(out_result, p->pid, p->name, current_time, finish_time, false);

        current_time = finish_time;
        p->completion_time = current_time;
        p->turnaround_time = p->completion_time - p->arrival_time;
        p->waiting_time = p->turnaround_time - p->burst_time;
        p->is_completed = true;
        total_burst += p->burst_time;
        completed++;
    }

    out_result->total_time = current_time;
    calculate_averages(out_result, total_burst);
    return 0;
}

/* 5. Round Robin (RR with configurable Quantum) */
int run_round_robin(const sched_process_t *inputs, int count, int quantum, sched_result_t *out_result) {
    if (!inputs || count <= 0 || !out_result) return -1;
    if (quantum <= 0) quantum = 2;

    memset(out_result, 0, sizeof(sched_result_t));
    out_result->algo = SCHED_ALGO_ROUND_ROBIN;
    out_result->algo_name = sched_algo_to_string(SCHED_ALGO_ROUND_ROBIN);
    out_result->process_count = count > MAX_SCHEDULE_PROCESSES ? MAX_SCHEDULE_PROCESSES : count;

    int total_burst = 0;
    for (int i = 0; i < out_result->process_count; i++) {
        out_result->processes[i] = inputs[i];
        out_result->processes[i].remaining_time = inputs[i].burst_time;
        out_result->processes[i].has_started = false;
        out_result->processes[i].is_completed = false;
        total_burst += inputs[i].burst_time;
    }

    /* Queue for Round Robin */
    int queue[MAX_SCHEDULE_PROCESSES * 16];
    int q_head = 0, q_tail = 0;
    bool in_queue[MAX_SCHEDULE_PROCESSES];
    memset(in_queue, 0, sizeof(in_queue));

    int current_time = 0;
    int completed = 0;

    /* Enqueue any process arriving at time 0 */
    for (int i = 0; i < out_result->process_count; i++) {
        if (out_result->processes[i].arrival_time <= current_time) {
            queue[q_tail++] = i;
            in_queue[i] = true;
        }
    }

    while (completed < out_result->process_count) {
        if (q_head == q_tail) {
            /* Queue is empty. Find earliest future arrival */
            int next_arrival = 1000000000;
            for (int i = 0; i < out_result->process_count; i++) {
                if (!out_result->processes[i].is_completed && out_result->processes[i].arrival_time < next_arrival) {
                    next_arrival = out_result->processes[i].arrival_time;
                }
            }
            if (next_arrival > current_time && next_arrival < 1000000000) {
                add_gantt_segment(out_result, -1, "IDLE", current_time, next_arrival, true);
                out_result->idle_time += (next_arrival - current_time);
                current_time = next_arrival;
                for (int i = 0; i < out_result->process_count; i++) {
                    if (!out_result->processes[i].is_completed &&
                        out_result->processes[i].arrival_time <= current_time &&
                        !in_queue[i]) {
                        queue[q_tail++] = i;
                        in_queue[i] = true;
                    }
                }
                continue;
            } else {
                break;
            }
        }

        int idx = queue[q_head++];
        in_queue[idx] = false;
        sched_process_t *p = &out_result->processes[idx];

        if (!p->has_started) {
            p->start_time = current_time;
            p->response_time = p->start_time - p->arrival_time;
            p->has_started = true;
        }

        int exec_time = (p->remaining_time > quantum) ? quantum : p->remaining_time;
        add_gantt_segment(out_result, p->pid, p->name, current_time, current_time + exec_time, false);

        p->remaining_time -= exec_time;
        int prev_time = current_time;
        current_time += exec_time;

        /* Enqueue newly arrived processes during this time interval */
        for (int i = 0; i < out_result->process_count; i++) {
            if (!out_result->processes[i].is_completed &&
                out_result->processes[i].arrival_time > prev_time &&
                out_result->processes[i].arrival_time <= current_time &&
                !in_queue[i] && i != idx) {
                queue[q_tail++] = i;
                in_queue[i] = true;
            }
        }

        if (p->remaining_time > 0) {
            queue[q_tail++] = idx;
            in_queue[idx] = true;
        } else {
            p->completion_time = current_time;
            p->turnaround_time = p->completion_time - p->arrival_time;
            p->waiting_time = p->turnaround_time - p->burst_time;
            p->is_completed = true;
            completed++;
        }
    }

    out_result->total_time = current_time;
    calculate_averages(out_result, total_burst);
    return 0;
}

/* Compare all and produce explainable recommendation */
int run_all_and_compare(const sched_process_t *inputs, int count, int quantum,
                        sched_comparison_t *out_comp) {
    if (!inputs || count <= 0 || !out_comp) return -1;
    memset(out_comp, 0, sizeof(sched_comparison_t));

    run_fcfs(inputs, count, &out_comp->results[SCHED_ALGO_FCFS]);
    run_sjf(inputs, count, &out_comp->results[SCHED_ALGO_SJF]);
    run_srtf(inputs, count, &out_comp->results[SCHED_ALGO_SRTF]);
    run_priority(inputs, count, false, &out_comp->results[SCHED_ALGO_PRIORITY]);
    run_round_robin(inputs, count, quantum, &out_comp->results[SCHED_ALGO_ROUND_ROBIN]);

    /* Calculate burst variance to analyze workload dispersion */
    double sum = 0.0;
    for (int i = 0; i < count; i++) {
        sum += inputs[i].burst_time;
    }
    double mean = sum / count;
    double var = 0.0;
    for (int i = 0; i < count; i++) {
        double d = inputs[i].burst_time - mean;
        var += d * d;
    }
    out_comp->variance_burst = var / count;

    /* Detect staggered arrivals */
    out_comp->has_staggered_arrivals = false;
    for (int i = 1; i < count; i++) {
        if (inputs[i].arrival_time != inputs[0].arrival_time) {
            out_comp->has_staggered_arrivals = true;
            break;
        }
    }

    /* Recommendation Logic */
    sched_algo_t best_algo = SCHED_ALGO_SRTF;
    double min_avg_wt = out_comp->results[SCHED_ALGO_SRTF].avg_waiting_time;

    for (int a = 0; a < SCHED_ALGO_COUNT; a++) {
        if (out_comp->results[a].avg_waiting_time < min_avg_wt) {
            min_avg_wt = out_comp->results[a].avg_waiting_time;
            best_algo = (sched_algo_t)a;
        }
    }

    out_comp->recommended_algo = best_algo;
    out_comp->recommended_name = sched_algo_to_string(best_algo);

    if (best_algo == SCHED_ALGO_SRTF) {
        snprintf(out_comp->recommendation_reason, sizeof(out_comp->recommendation_reason),
            "Shortest Remaining Time First (SRTF) achieved the lowest average waiting time (%.2f ms) "
            "and turnaround time (%.2f ms). Its preemptive policy effectively avoids the convoy effect "
            "caused by long CPU bursts.",
            out_comp->results[SCHED_ALGO_SRTF].avg_waiting_time,
            out_comp->results[SCHED_ALGO_SRTF].avg_turnaround_time);
    } else if (best_algo == SCHED_ALGO_SJF) {
        snprintf(out_comp->recommendation_reason, sizeof(out_comp->recommendation_reason),
            "Shortest Job First (SJF) is optimal for this batch workload without context-switching overhead, "
            "yielding minimal average waiting time (%.2f ms) without requiring preemptive scheduling.",
            out_comp->results[SCHED_ALGO_SJF].avg_waiting_time);
    } else if (best_algo == SCHED_ALGO_ROUND_ROBIN) {
        snprintf(out_comp->recommendation_reason, sizeof(out_comp->recommendation_reason),
            "Round Robin (Quantum = %d) provides superior fairness and bounded response time (%.2f ms), "
            "preventing process starvation across uniform or interactive workloads.",
            quantum, out_comp->results[SCHED_ALGO_ROUND_ROBIN].avg_response_time);
    } else if (best_algo == SCHED_ALGO_PRIORITY) {
        snprintf(out_comp->recommendation_reason, sizeof(out_comp->recommendation_reason),
            "Priority Scheduling aligns execution order with designated process urgency levels while "
            "maintaining an average waiting time of %.2f ms.",
            out_comp->results[SCHED_ALGO_PRIORITY].avg_waiting_time);
    } else {
        snprintf(out_comp->recommendation_reason, sizeof(out_comp->recommendation_reason),
            "First Come First Serve (FCFS) provides simple, deterministic scheduling with zero preemption overhead "
            "and average waiting time of %.2f ms.",
            out_comp->results[SCHED_ALGO_FCFS].avg_waiting_time);
    }

    return 0;
}
