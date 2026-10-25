#define __USE_MINGW_ANSI_STDIO 1
#include "routes.h"
#include "cpu_monitor.h"
#include "memory_monitor.h"
#include "process_monitor.h"
#include "cpu_scheduler.h"
#include "memory_prediction.h"
#include "page_replacement.h"
#include "deadlock.h"
#include "log_analyzer.h"
#include "resource_advisory.h"
#include "json_utils.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>

void http_response_init(http_response_t *res) {
    if (!res) return;
    res->status_code = 200;
    res->status_text = "OK";
    res->content_type = "application/json";
    res->body = NULL;
    res->body_len = 0;
}

void http_response_free(http_response_t *res) {
    if (!res) return;
    if (res->body) {
        free(res->body);
        res->body = NULL;
    }
    res->body_len = 0;
}

void http_response_set_json(http_response_t *res, int status_code, json_builder_t *jb) {
    res->status_code = status_code;
    res->status_text = (status_code == 200) ? "OK" :
                       (status_code == 400) ? "Bad Request" :
                       (status_code == 404) ? "Not Found" : "Internal Server Error";
    res->content_type = "application/json";

    const char *str = json_get_string(jb);
    size_t len = strlen(str);
    res->body = (char *)malloc(len + 1);
    if (res->body) {
        memcpy(res->body, str, len + 1);
        res->body_len = len;
    }
}

void http_response_set_error(http_response_t *res, int status_code, const char *error_msg) {
    json_builder_t jb;
    json_init(&jb, 256);
    json_obj_start(&jb);
    json_add_int(&jb, "status", status_code);
    json_add_string(&jb, "error", error_msg ? error_msg : "Unknown error");
    json_add_uint64(&jb, "timestamp", (uint64_t)time(NULL));
    json_obj_end(&jb);

    http_response_set_json(res, status_code, &jb);
    json_free(&jb);
}

/* ==================== Query Parameter Helpers ==================== */
static int parse_query_int(const char *query, const char *key, int default_val) {
    if (!query || !key) return default_val;
    char search_key[64];
    snprintf(search_key, sizeof(search_key), "%s=", key);
    const char *p = strstr(query, search_key);
    if (!p) return default_val;
    p += strlen(search_key);
    return atoi(p);
}

static double parse_query_double(const char *query, const char *key, double default_val) {
    if (!query || !key) return default_val;
    char search_key[64];
    snprintf(search_key, sizeof(search_key), "%s=", key);
    const char *p = strstr(query, search_key);
    if (!p) return default_val;
    p += strlen(search_key);
    return atof(p);
}

/* ==================== Phase 1 Existing Routes ==================== */

static void route_health(http_response_t *res) {
    json_builder_t jb;
    json_init(&jb, 512);

    json_obj_start(&jb);
    json_add_string(&jb, "status", "ok");
    json_add_string(&jb, "service", "Intelligent OS Analytics Engine");
    json_add_string(&jb, "version", "3.0.0");
    json_add_string(&jb, "phase", "Phase 3 - Deadlock Analysis, OS Log Analysis & Advisory Resource Allocation");
#if defined(__linux__)
    json_add_string(&jb, "platform", "Linux (Ubuntu/Kernel)");
#elif defined(_WIN32)
    json_add_string(&jb, "platform", "Windows Host (MinGW GCC)");
#else
    json_add_string(&jb, "platform", "POSIX");
#endif
    json_add_uint64(&jb, "uptime_seconds", get_system_uptime_seconds());
    json_add_uint64(&jb, "timestamp", (uint64_t)time(NULL));
    json_obj_end(&jb);

    http_response_set_json(res, 200, &jb);
    json_free(&jb);
}

static void route_system_overview(http_response_t *res) {
    cpu_stats_t cpu;
    memset(&cpu, 0, sizeof(cpu));
    cpu_monitor_update(&cpu);

    memory_stats_t mem;
    memset(&mem, 0, sizeof(mem));
    memory_monitor_update(&mem);

    /* Record memory sample for continuous forecasting */
    mem_history_record_sample(mem.utilization_pct, mem.used_kb, mem.available_kb, mem.total_kb);

    process_table_t procs;
    process_monitor_update(&procs);

    json_builder_t jb;
    json_init(&jb, 1024);

    json_obj_start(&jb);
    json_add_double(&jb, "cpu_utilization", cpu.overall_utilization, 2);
    json_add_int(&jb, "cpu_cores", cpu.core_count);
    json_add_string(&jb, "cpu_model", cpu.model_name);

    json_add_uint64(&jb, "memory_total_kb", mem.total_kb);
    json_add_uint64(&jb, "memory_available_kb", mem.available_kb);
    json_add_uint64(&jb, "memory_used_kb", mem.used_kb);
    json_add_uint64(&jb, "memory_free_kb", mem.free_kb);
    json_add_double(&jb, "memory_utilization", mem.utilization_pct, 2);

    json_add_int(&jb, "total_processes", procs.total_processes);
    json_add_int(&jb, "running_processes", procs.running_count);
    json_add_int(&jb, "sleeping_processes", procs.sleeping_count);
    json_add_int(&jb, "stopped_processes", procs.stopped_count);
    json_add_int(&jb, "zombie_processes", procs.zombie_count);
    json_add_uint64(&jb, "uptime_seconds", procs.system_uptime_seconds);
    json_add_uint64(&jb, "timestamp", (uint64_t)time(NULL));
    json_obj_end(&jb);

    process_table_free(&procs);

    http_response_set_json(res, 200, &jb);
    json_free(&jb);
}

static void route_system_cpu(http_response_t *res) {
    cpu_stats_t cpu;
    memset(&cpu, 0, sizeof(cpu));
    cpu_monitor_update(&cpu);

    json_builder_t jb;
    json_init(&jb, 2048);

    json_obj_start(&jb);
    json_add_double(&jb, "utilization", cpu.overall_utilization, 2);
    json_add_int(&jb, "cores", cpu.core_count);
    json_add_int(&jb, "user_hz", cpu.user_hz);
    json_add_uint64(&jb, "total_jiffies", cpu.total_jiffies);
    json_add_uint64(&jb, "idle_jiffies", cpu.idle_jiffies);
    json_add_string(&jb, "model_name", cpu.model_name);

    json_key(&jb, "per_core");
    json_arr_start(&jb);
    for (int i = 0; i < cpu.active_cores; i++) {
        json_obj_start(&jb);
        json_add_int(&jb, "core_id", cpu.cores[i].core_id);
        json_add_double(&jb, "utilization_pct", cpu.cores[i].utilization_pct, 2);
        json_obj_end(&jb);
    }
    json_arr_end(&jb);

    json_add_uint64(&jb, "timestamp", (uint64_t)time(NULL));
    json_obj_end(&jb);

    http_response_set_json(res, 200, &jb);
    json_free(&jb);
}

static void route_system_memory(http_response_t *res) {
    memory_stats_t mem;
    memset(&mem, 0, sizeof(mem));
    memory_monitor_update(&mem);

    mem_history_record_sample(mem.utilization_pct, mem.used_kb, mem.available_kb, mem.total_kb);

    json_builder_t jb;
    json_init(&jb, 1024);

    json_obj_start(&jb);
    json_add_uint64(&jb, "total_kb", mem.total_kb);
    json_add_uint64(&jb, "available_kb", mem.available_kb);
    json_add_uint64(&jb, "used_kb", mem.used_kb);
    json_add_uint64(&jb, "free_kb", mem.free_kb);
    json_add_double(&jb, "utilization_pct", mem.utilization_pct, 2);
    json_add_uint64(&jb, "buffers_kb", mem.buffers_kb);
    json_add_uint64(&jb, "cached_kb", mem.cached_kb);
    json_add_uint64(&jb, "swap_total_kb", mem.swap_total_kb);
    json_add_uint64(&jb, "swap_free_kb", mem.swap_free_kb);
    json_add_uint64(&jb, "swap_used_kb", mem.swap_used_kb);
    json_add_uint64(&jb, "timestamp", (uint64_t)time(NULL));
    json_obj_end(&jb);

    http_response_set_json(res, 200, &jb);
    json_free(&jb);
}

static void route_processes(http_response_t *res) {
    process_table_t procs;
    process_monitor_update(&procs);

    json_builder_t jb;
    json_init(&jb, 65536);

    json_obj_start(&jb);
    json_add_int(&jb, "total", procs.total_processes);
    json_add_int(&jb, "count", procs.count);
    json_add_int(&jb, "running", procs.running_count);
    json_add_int(&jb, "sleeping", procs.sleeping_count);
    json_add_int(&jb, "stopped", procs.stopped_count);
    json_add_int(&jb, "zombie", procs.zombie_count);
    json_add_uint64(&jb, "uptime_seconds", procs.system_uptime_seconds);

    json_key(&jb, "processes");
    json_arr_start(&jb);
    for (int i = 0; i < procs.count; i++) {
        process_info_t *p = &procs.processes[i];
        json_obj_start(&jb);
        json_add_int(&jb, "pid", p->pid);
        json_add_int(&jb, "ppid", p->ppid);
        json_add_string(&jb, "name", p->name);
        char state_buf[2] = { p->state_char, '\0' };
        json_add_string(&jb, "state", state_buf);
        json_add_string(&jb, "state_full", p->state_str);
        json_add_double(&jb, "cpu_pct", p->cpu_pct, 2);
        json_add_uint64(&jb, "mem_kb", p->mem_rss_kb);
        json_add_uint64(&jb, "vsz_kb", p->mem_vsz_kb);
        json_add_double(&jb, "mem_pct", p->mem_pct, 2);
        json_add_int(&jb, "threads", p->num_threads);
        json_add_int(&jb, "priority", p->priority);
        json_add_int(&jb, "nice", p->nice_val);
        json_obj_end(&jb);
    }
    json_arr_end(&jb);

    json_add_uint64(&jb, "timestamp", (uint64_t)time(NULL));
    json_obj_end(&jb);

    process_table_free(&procs);

    http_response_set_json(res, 200, &jb);
    json_free(&jb);
}

static void route_process_detail(const char *pid_str, http_response_t *res) {
    if (!pid_str || *pid_str == '\0') {
        http_response_set_error(res, 400, "Missing PID parameter");
        return;
    }

    for (const char *p = pid_str; *p; p++) {
        if (!isdigit((unsigned char)*p)) {
            http_response_set_error(res, 400, "Invalid PID format: must be numeric integer");
            return;
        }
    }

    int pid = atoi(pid_str);
    if (pid <= 0) {
        http_response_set_error(res, 400, "Invalid PID: must be positive integer");
        return;
    }

    process_info_t proc;
    memset(&proc, 0, sizeof(proc));
    if (process_get_by_pid(pid, &proc) != 0) {
        char err_msg[128];
        snprintf(err_msg, sizeof(err_msg), "Process with PID %d not found or terminated", pid);
        http_response_set_error(res, 404, err_msg);
        return;
    }

    json_builder_t jb;
    json_init(&jb, 2048);

    json_obj_start(&jb);
    json_add_int(&jb, "pid", proc.pid);
    json_add_int(&jb, "ppid", proc.ppid);
    json_add_string(&jb, "name", proc.name);
    char state_buf[2] = { proc.state_char, '\0' };
    json_add_string(&jb, "state", state_buf);
    json_add_string(&jb, "state_full", proc.state_str);
    json_add_double(&jb, "cpu_pct", proc.cpu_pct, 2);
    json_add_uint64(&jb, "mem_rss_kb", proc.mem_rss_kb);
    json_add_uint64(&jb, "mem_vsz_kb", proc.mem_vsz_kb);
    json_add_double(&jb, "mem_pct", proc.mem_pct, 2);
    json_add_int(&jb, "threads", proc.num_threads);
    json_add_int(&jb, "priority", proc.priority);
    json_add_int(&jb, "nice", proc.nice_val);
    json_add_string(&jb, "cmdline", proc.cmdline);
    json_add_uint64(&jb, "utime_ticks", proc.utime_ticks);
    json_add_uint64(&jb, "stime_ticks", proc.stime_ticks);
    json_add_uint64(&jb, "start_time_seconds", proc.start_time_seconds);
    json_add_uint64(&jb, "timestamp", (uint64_t)time(NULL));
    json_obj_end(&jb);

    http_response_set_json(res, 200, &jb);
    json_free(&jb);
}

/* ==================== Phase 2: CPU Scheduling Routes ==================== */

static void serialize_sched_result(json_builder_t *jb, const sched_result_t *res) {
    json_obj_start(jb);
    json_add_int(jb, "algo_id", res->algo);
    json_add_string(jb, "algo_name", res->algo_name);
    json_add_double(jb, "avg_waiting_time", res->avg_waiting_time, 2);
    json_add_double(jb, "avg_turnaround_time", res->avg_turnaround_time, 2);
    json_add_double(jb, "avg_response_time", res->avg_response_time, 2);
    json_add_double(jb, "cpu_utilization_pct", res->cpu_utilization_pct, 2);
    json_add_double(jb, "throughput", res->throughput, 4);
    json_add_int(jb, "total_time", res->total_time);
    json_add_int(jb, "idle_time", res->idle_time);

    /* Process Table */
    json_key(jb, "processes");
    json_arr_start(jb);
    for (int i = 0; i < res->process_count; i++) {
        const sched_process_t *p = &res->processes[i];
        json_obj_start(jb);
        json_add_int(jb, "pid", p->pid);
        json_add_string(jb, "name", p->name);
        json_add_int(jb, "arrival_time", p->arrival_time);
        json_add_int(jb, "burst_time", p->burst_time);
        json_add_int(jb, "priority", p->priority);
        json_add_int(jb, "start_time", p->start_time);
        json_add_int(jb, "completion_time", p->completion_time);
        json_add_int(jb, "turnaround_time", p->turnaround_time);
        json_add_int(jb, "waiting_time", p->waiting_time);
        json_add_int(jb, "response_time", p->response_time);
        json_obj_end(jb);
    }
    json_arr_end(jb);

    /* Gantt Chart Segments */
    json_key(jb, "gantt");
    json_arr_start(jb);
    for (int i = 0; i < res->gantt_count; i++) {
        const gantt_segment_t *g = &res->gantt[i];
        json_obj_start(jb);
        json_add_int(jb, "pid", g->pid);
        json_add_string(jb, "name", g->name);
        json_add_int(jb, "start_time", g->start_time);
        json_add_int(jb, "end_time", g->end_time);
        json_add_bool(jb, "is_idle", g->is_idle);
        json_obj_end(jb);
    }
    json_arr_end(jb);

    json_obj_end(jb);
}

/* Lightweight parser for scheduling JSON body */
static int parse_scheduling_payload(const char *body, sched_process_t *out_procs,
                                   int max_procs, int *out_count, int *out_quantum) {
    if (!body || !out_procs || max_procs <= 0) return -1;

    int quantum = 2;
    const char *q_ptr = strstr(body, "\"quantum\"");
    if (q_ptr) {
        q_ptr = strchr(q_ptr, ':');
        if (q_ptr) {
            quantum = atoi(q_ptr + 1);
            if (quantum <= 0) quantum = 2;
        }
    }
    if (out_quantum) *out_quantum = quantum;

    const char *proc_arr = strstr(body, "\"processes\"");
    if (!proc_arr) return -1;
    proc_arr = strchr(proc_arr, '[');
    if (!proc_arr) return -1;

    int count = 0;
    const char *curr = proc_arr + 1;

    while (*curr && *curr != ']' && count < max_procs) {
        const char *obj_start = strchr(curr, '{');
        if (!obj_start) break;
        const char *obj_end = strchr(obj_start, '}');
        if (!obj_end) break;

        sched_process_t p;
        memset(&p, 0, sizeof(p));
        p.pid = count + 1;
        p.priority = 1;
        snprintf(p.name, sizeof(p.name), "P%d", count + 1);

        /* Parse pid */
        const char *f = strstr(obj_start, "\"pid\"");
        if (f && f < obj_end) {
            f = strchr(f, ':');
            if (f) p.pid = atoi(f + 1);
        }

        /* Parse name */
        f = strstr(obj_start, "\"name\"");
        if (f && f < obj_end) {
            f = strchr(f, ':');
            if (f) {
                const char *quote1 = strchr(f, '"');
                if (quote1 && quote1 < obj_end) {
                    const char *quote2 = strchr(quote1 + 1, '"');
                    if (quote2 && quote2 < obj_end) {
                        size_t nlen = quote2 - quote1 - 1;
                        if (nlen >= sizeof(p.name)) nlen = sizeof(p.name) - 1;
                        memcpy(p.name, quote1 + 1, nlen);
                        p.name[nlen] = '\0';
                    }
                }
            }
        }

        /* Parse arrival_time / arrival */
        f = strstr(obj_start, "\"arrival_time\"");
        if (!f || f > obj_end) f = strstr(obj_start, "\"arrival\"");
        if (f && f < obj_end) {
            f = strchr(f, ':');
            if (f) p.arrival_time = atoi(f + 1);
        }

        /* Parse burst_time / burst */
        f = strstr(obj_start, "\"burst_time\"");
        if (!f || f > obj_end) f = strstr(obj_start, "\"burst\"");
        if (f && f < obj_end) {
            f = strchr(f, ':');
            if (f) p.burst_time = atoi(f + 1);
        }

        /* Parse priority */
        f = strstr(obj_start, "\"priority\"");
        if (f && f < obj_end) {
            f = strchr(f, ':');
            if (f) p.priority = atoi(f + 1);
        }

        if (p.burst_time > 0 && p.arrival_time >= 0) {
            out_procs[count++] = p;
        }

        curr = obj_end + 1;
    }

    if (out_count) *out_count = count;
    return (count > 0) ? 0 : -1;
}

static void route_scheduling_simulate(const char *body, http_response_t *res) {
    if (!body || strlen(body) < 10) {
        http_response_set_error(res, 400, "Request body must contain valid processes array");
        return;
    }

    sched_process_t procs[MAX_SCHEDULE_PROCESSES];
    int count = 0;
    int quantum = 2;

    if (parse_scheduling_payload(body, procs, MAX_SCHEDULE_PROCESSES, &count, &quantum) != 0 || count <= 0) {
        http_response_set_error(res, 400, "Failed to parse workload processes. Ensure burst_time > 0.");
        return;
    }

    sched_comparison_t comp;
    if (run_all_and_compare(procs, count, quantum, &comp) != 0) {
        http_response_set_error(res, 500, "Scheduling simulation failed");
        return;
    }

    json_builder_t jb;
    json_init(&jb, 16384);

    json_obj_start(&jb);
    json_add_int(&jb, "quantum", quantum);
    json_add_int(&jb, "process_count", count);
    json_add_int(&jb, "recommended_algo_id", comp.recommended_algo);
    json_add_string(&jb, "recommended_algo_name", comp.recommended_name);
    json_add_string(&jb, "recommendation_reason", comp.recommendation_reason);
    json_add_double(&jb, "burst_variance", comp.variance_burst, 2);
    json_add_bool(&jb, "has_staggered_arrivals", comp.has_staggered_arrivals);

    /* Serialized algorithm results */
    json_key(&jb, "algorithms");
    json_obj_start(&jb);

    json_key(&jb, "fcfs");
    serialize_sched_result(&jb, &comp.results[SCHED_ALGO_FCFS]);

    json_key(&jb, "sjf");
    serialize_sched_result(&jb, &comp.results[SCHED_ALGO_SJF]);

    json_key(&jb, "srtf");
    serialize_sched_result(&jb, &comp.results[SCHED_ALGO_SRTF]);

    json_key(&jb, "priority");
    serialize_sched_result(&jb, &comp.results[SCHED_ALGO_PRIORITY]);

    json_key(&jb, "round_robin");
    serialize_sched_result(&jb, &comp.results[SCHED_ALGO_ROUND_ROBIN]);

    json_obj_end(&jb); /* algorithms */

    json_add_uint64(&jb, "timestamp", (uint64_t)time(NULL));
    json_obj_end(&jb);

    http_response_set_json(res, 200, &jb);
    json_free(&jb);
}

static void route_scheduling_examples(http_response_t *res) {
    json_builder_t jb;
    json_init(&jb, 4096);

    json_obj_start(&jb);
    json_key(&jb, "examples");
    json_arr_start(&jb);

    /* Example 1: Convoy Effect Workload */
    json_obj_start(&jb);
    json_add_string(&jb, "id", "convoy");
    json_add_string(&jb, "title", "Convoy Effect (SJF/SRTF Advantage)");
    json_add_string(&jb, "description", "One long CPU-bound process arriving first, followed by short interactive tasks.");
    json_add_int(&jb, "quantum", 2);
    json_key(&jb, "processes");
    json_arr_start(&jb);
    json_add_raw(&jb, NULL, "{\"pid\":1,\"name\":\"P1\",\"arrival_time\":0,\"burst_time\":24,\"priority\":3}");
    json_add_raw(&jb, NULL, "{\"pid\":2,\"name\":\"P2\",\"arrival_time\":1,\"burst_time\":3,\"priority\":1}");
    json_add_raw(&jb, NULL, "{\"pid\":3,\"name\":\"P3\",\"arrival_time\":2,\"burst_time\":3,\"priority\":2}");
    json_arr_end(&jb);
    json_obj_end(&jb);

    /* Example 2: Interactive Round Robin Workload */
    json_obj_start(&jb);
    json_add_string(&jb, "id", "interactive");
    json_add_string(&jb, "title", "Time-Sharing Interactive Workload");
    json_add_string(&jb, "description", "Multiple staggered processes benefiting from fair time slicing.");
    json_add_int(&jb, "quantum", 3);
    json_key(&jb, "processes");
    json_arr_start(&jb);
    json_add_raw(&jb, NULL, "{\"pid\":1,\"name\":\"P1\",\"arrival_time\":0,\"burst_time\":6,\"priority\":2}");
    json_add_raw(&jb, NULL, "{\"pid\":2,\"name\":\"P2\",\"arrival_time\":2,\"burst_time\":8,\"priority\":1}");
    json_add_raw(&jb, NULL, "{\"pid\":3,\"name\":\"P3\",\"arrival_time\":4,\"burst_time\":7,\"priority\":3}");
    json_add_raw(&jb, NULL, "{\"pid\":4,\"name\":\"P4\",\"arrival_time\":5,\"burst_time\":3,\"priority\":2}");
    json_arr_end(&jb);
    json_obj_end(&jb);

    /* Example 3: Priority Scheduling Workload */
    json_obj_start(&jb);
    json_add_string(&jb, "id", "priority_driven");
    json_add_string(&jb, "title", "Priority-Driven System Workload");
    json_add_string(&jb, "description", "Mix of real-time critical and background batch tasks.");
    json_add_int(&jb, "quantum", 2);
    json_key(&jb, "processes");
    json_arr_start(&jb);
    json_add_raw(&jb, NULL, "{\"pid\":1,\"name\":\"P1\",\"arrival_time\":0,\"burst_time\":10,\"priority\":3}");
    json_add_raw(&jb, NULL, "{\"pid\":2,\"name\":\"P2\",\"arrival_time\":0,\"burst_time\":1,\"priority\":1}");
    json_add_raw(&jb, NULL, "{\"pid\":3,\"name\":\"P3\",\"arrival_time\":0,\"burst_time\":2,\"priority\":4}");
    json_add_raw(&jb, NULL, "{\"pid\":4,\"name\":\"P4\",\"arrival_time\":0,\"burst_time\":1,\"priority\":5}");
    json_add_raw(&jb, NULL, "{\"pid\":5,\"name\":\"P5\",\"arrival_time\":0,\"burst_time\":5,\"priority\":2}");
    json_arr_end(&jb);
    json_obj_end(&jb);

    json_arr_end(&jb);
    json_obj_end(&jb);

    http_response_set_json(res, 200, &jb);
    json_free(&jb);
}

/* ==================== Phase 2: Memory Prediction Routes ==================== */

static void route_memory_forecast(const char *query, http_response_t *res) {
    int horizon = parse_query_int(query, "horizon", 60);
    int window = parse_query_int(query, "window", 10);
    double threshold = parse_query_double(query, "threshold", 85.0);

    mem_forecast_result_t fc;
    if (mem_predict_forecast(horizon, window, threshold, &fc) != 0) {
        http_response_set_error(res, 500, "Memory prediction calculation failed");
        return;
    }

    json_builder_t jb;
    json_init(&jb, 4096);

    json_obj_start(&jb);
    json_add_int(&jb, "trend_id", (int)fc.trend);
    json_add_string(&jb, "trend_label", fc.trend_label);
    json_add_double(&jb, "slope_pct_per_min", fc.slope_pct_per_min, 4);
    json_add_double(&jb, "r_squared", fc.r_squared, 4);
    json_add_double(&jb, "current_utilization_pct", fc.current_utilization_pct, 2);
    json_add_double(&jb, "predicted_sma_util_pct", fc.predicted_sma_util_pct, 2);
    json_add_double(&jb, "predicted_reg_util_pct", fc.predicted_reg_util_pct, 2);
    json_add_uint64(&jb, "current_available_kb", fc.current_available_kb);
    json_add_uint64(&jb, "predicted_available_kb", fc.predicted_available_kb);
    json_add_uint64(&jb, "total_ram_kb", fc.total_ram_kb);
    json_add_int(&jb, "forecast_horizon_seconds", fc.forecast_horizon_seconds);
    json_add_int(&jb, "sample_count", fc.sample_count);
    json_add_bool(&jb, "threshold_exceeded", fc.threshold_exceeded);
    json_add_double(&jb, "threshold_pct", fc.threshold_pct, 1);
    json_add_int(&jb, "seconds_to_exhaustion", fc.seconds_to_exhaustion);
    json_add_string(&jb, "warning_message", fc.warning_message);
    json_add_string(&jb, "limitation_explanation", fc.limitation_explanation);

    /* Forecast points */
    json_key(&jb, "forecast_points");
    json_arr_start(&jb);
    for (int i = 0; i < fc.point_count; i++) {
        json_obj_start(&jb);
        json_add_uint64(&jb, "timestamp", fc.points[i].timestamp);
        json_add_int(&jb, "offset_seconds", fc.points[i].offset_seconds);
        json_add_double(&jb, "predicted_util_pct", fc.points[i].predicted_util_pct, 2);
        json_add_uint64(&jb, "estimated_used_kb", fc.points[i].estimated_used_kb);
        json_add_uint64(&jb, "estimated_available_kb", fc.points[i].estimated_available_kb);
        json_obj_end(&jb);
    }
    json_arr_end(&jb);

    json_add_uint64(&jb, "timestamp", (uint64_t)time(NULL));
    json_obj_end(&jb);

    http_response_set_json(res, 200, &jb);
    json_free(&jb);
}

static void route_memory_history(http_response_t *res) {
    mem_sample_t samples[MAX_MEM_HISTORY];
    int count = 0;
    mem_history_get_samples(samples, MAX_MEM_HISTORY, &count);

    json_builder_t jb;
    json_init(&jb, 8192);

    json_obj_start(&jb);
    json_add_int(&jb, "count", count);

    json_key(&jb, "samples");
    json_arr_start(&jb);
    for (int i = 0; i < count; i++) {
        json_obj_start(&jb);
        json_add_uint64(&jb, "timestamp", samples[i].timestamp);
        json_add_double(&jb, "utilization_pct", samples[i].utilization_pct, 2);
        json_add_uint64(&jb, "used_kb", samples[i].used_kb);
        json_add_uint64(&jb, "available_kb", samples[i].available_kb);
        json_add_uint64(&jb, "total_kb", samples[i].total_kb);
        json_obj_end(&jb);
    }
    json_arr_end(&jb);

    json_add_uint64(&jb, "timestamp", (uint64_t)time(NULL));
    json_obj_end(&jb);

    http_response_set_json(res, 200, &jb);
    json_free(&jb);
}

/* ==================== Phase 2: Page Replacement Routes ==================== */

static void serialize_page_result(json_builder_t *jb, const page_sim_result_t *res) {
    json_obj_start(jb);
    json_add_int(jb, "algo_id", res->algo);
    json_add_string(jb, "algo_name", res->algo_name);
    json_add_int(jb, "frame_capacity", res->frame_capacity);
    json_add_int(jb, "total_references", res->total_references);
    json_add_int(jb, "hit_count", res->hit_count);
    json_add_int(jb, "fault_count", res->fault_count);
    json_add_double(jb, "hit_ratio", res->hit_ratio, 4);
    json_add_double(jb, "fault_ratio", res->fault_ratio, 4);

    /* Frame by Frame Steps */
    json_key(jb, "steps");
    json_arr_start(jb);
    for (int i = 0; i < res->step_count; i++) {
        const page_step_t *s = &res->steps[i];
        json_obj_start(jb);
        json_add_int(jb, "step", s->step);
        json_add_int(jb, "page", s->page);
        json_add_bool(jb, "is_hit", s->is_hit);
        json_add_int(jb, "victim_page", s->victim_page);
        json_add_int(jb, "replaced_frame_idx", s->replaced_frame_idx);

        json_key(jb, "frames");
        json_arr_start(jb);
        for (int f = 0; f < s->frame_count; f++) {
            json_arr_add_int(jb, s->frames[f]);
        }
        json_arr_end(jb);

        json_obj_end(jb);
    }
    json_arr_end(jb);

    json_obj_end(jb);
}

static int parse_page_payload(const char *body, int *out_refs, int max_refs, int *out_count, int *out_frames) {
    if (!body || !out_refs || max_refs <= 0) return -1;

    int frames = 3;
    const char *f_ptr = strstr(body, "\"frame_count\"");
    if (!f_ptr) f_ptr = strstr(body, "\"frames\"");
    if (f_ptr) {
        f_ptr = strchr(f_ptr, ':');
        if (f_ptr) {
            int val = atoi(f_ptr + 1);
            if (val <= 0 || val > MAX_FRAMES) {
                return -2; /* Invalid frame_count parameter */
            }
            frames = val;
        }
    }
    if (out_frames) *out_frames = frames;

    const char *arr = strstr(body, "\"reference_string\"");
    if (!arr) arr = strstr(body, "\"references\"");
    if (!arr) return -1;

    int count = 0;
    const char *p = strchr(arr, '[');
    if (p) {
        p++;
        while (*p && *p != ']' && count < max_refs) {
            while (*p && (*p == ' ' || *p == ',' || *p == '\t' || *p == '\n' || *p == '\r')) p++;
            if (*p >= '0' && *p <= '9') {
                out_refs[count++] = atoi(p);
                while (*p >= '0' && *p <= '9') p++;
            } else {
                p++;
            }
        }
    } else {
        /* Check if comma-separated string */
        p = strchr(arr, '"');
        if (p) {
            p = strchr(p + 1, '"'); /* after key */
            if (p) {
                p = strchr(p + 1, '"'); /* start of string */
                if (p) {
                    p++;
                    while (*p && *p != '"' && count < max_refs) {
                        while (*p && (*p == ' ' || *p == ',')) p++;
                        if (*p >= '0' && *p <= '9') {
                            out_refs[count++] = atoi(p);
                            while (*p >= '0' && *p <= '9') p++;
                        } else {
                            p++;
                        }
                    }
                }
            }
        }
    }

    if (out_count) *out_count = count;
    return (count > 0) ? 0 : -1;
}

static void route_page_simulate(const char *body, http_response_t *res) {
    if (!body || strlen(body) < 10) {
        http_response_set_error(res, 400, "Request body must contain frames and reference_string array");
        return;
    }

    int refs[MAX_PAGE_REFS];
    int count = 0;
    int frames = 3;

    int parse_res = parse_page_payload(body, refs, MAX_PAGE_REFS, &count, &frames);
    if (parse_res == -2) {
        http_response_set_error(res, 400, "Invalid frame_count: must be between 1 and 16");
        return;
    }
    if (parse_res != 0 || count <= 0) {
        http_response_set_error(res, 400, "Invalid reference string. Provide at least 1 page number.");
        return;
    }

    page_comparison_t comp;
    if (run_all_page_replacement(refs, count, frames, &comp) != 0) {
        http_response_set_error(res, 500, "Page replacement simulation failed");
        return;
    }

    json_builder_t jb;
    json_init(&jb, 16384);

    json_obj_start(&jb);
    json_add_int(&jb, "frames", frames);
    json_add_int(&jb, "reference_count", count);
    json_add_int(&jb, "best_algo_id", comp.best_algo);
    json_add_string(&jb, "best_algo_name", comp.best_algo_name);
    json_add_int(&jb, "min_faults", comp.min_faults);
    json_add_string(&jb, "comparison_summary", comp.comparison_summary);

    json_key(&jb, "algorithms");
    json_obj_start(&jb);

    json_key(&jb, "fifo");
    serialize_page_result(&jb, &comp.results[PAGE_ALGO_FIFO]);

    json_key(&jb, "lru");
    serialize_page_result(&jb, &comp.results[PAGE_ALGO_LRU]);

    json_key(&jb, "optimal");
    serialize_page_result(&jb, &comp.results[PAGE_ALGO_OPTIMAL]);

    json_obj_end(&jb); /* algorithms */

    json_add_uint64(&jb, "timestamp", (uint64_t)time(NULL));
    json_obj_end(&jb);

    http_response_set_json(res, 200, &jb);
    json_free(&jb);
}

static void route_page_examples(http_response_t *res) {
    json_builder_t jb;
    json_init(&jb, 2048);

    json_obj_start(&jb);
    json_key(&jb, "examples");
    json_arr_start(&jb);

    /* Example 1: Standard Textbook */
    json_obj_start(&jb);
    json_add_string(&jb, "id", "standard");
    json_add_string(&jb, "title", "Silberschatz Standard Reference String");
    json_add_string(&jb, "description", "Classic 20-reference sequence demonstrating FIFO, LRU, and Optimal differences.");
    json_add_int(&jb, "frames", 3);
    json_key(&jb, "references");
    json_arr_start(&jb);
    int refs1[] = { 7, 0, 1, 2, 0, 3, 0, 4, 2, 3, 0, 3, 2, 1, 2, 0, 1, 7, 0, 1 };
    for (size_t i = 0; i < sizeof(refs1)/sizeof(refs1[0]); i++) json_arr_add_int(&jb, refs1[i]);
    json_arr_end(&jb);
    json_obj_end(&jb);

    /* Example 2: Belady's Anomaly Reference String */
    json_obj_start(&jb);
    json_add_string(&jb, "id", "belady");
    json_add_string(&jb, "title", "Belady's Anomaly (FIFO Anomaly Demo)");
    json_add_string(&jb, "description", "FIFO experiences 9 faults with 3 frames, but 10 faults with 4 frames!");
    json_add_int(&jb, "frames", 3);
    json_key(&jb, "references");
    json_arr_start(&jb);
    int refs2[] = { 1, 2, 3, 4, 1, 2, 5, 1, 2, 3, 4, 5 };
    for (size_t i = 0; i < sizeof(refs2)/sizeof(refs2[0]); i++) json_arr_add_int(&jb, refs2[i]);
    json_arr_end(&jb);
    json_obj_end(&jb);

    /* Example 3: High Locality of Reference */
    json_obj_start(&jb);
    json_add_string(&jb, "id", "locality");
    json_add_string(&jb, "title", "High Temporal Locality Workload");
    json_add_string(&jb, "description", "Working set with repeated loops where LRU achieves very high hit ratios.");
    json_add_int(&jb, "frames", 4);
    json_key(&jb, "references");
    json_arr_start(&jb);
    int refs3[] = { 2, 3, 2, 1, 5, 2, 4, 5, 3, 2, 5, 2 };
    for (size_t i = 0; i < sizeof(refs3)/sizeof(refs3[0]); i++) json_arr_add_int(&jb, refs3[i]);
    json_arr_end(&jb);
    json_obj_end(&jb);

    json_arr_end(&jb);
    json_obj_end(&jb);

    http_response_set_json(res, 200, &jb);
    json_free(&jb);
}

/* ==================== Phase 3: Deadlock Analysis Routes ==================== */

static int parse_int_array(const char *json_arr_start, int *out_arr, int max_len) {
    if (!json_arr_start || !out_arr) return 0;
    const char *p = strchr(json_arr_start, '[');
    if (!p) return 0;
    p++;
    int count = 0;
    while (*p && *p != ']' && count < max_len) {
        while (*p && (*p == ' ' || *p == '\t' || *p == ',' || *p == '\r' || *p == '\n')) p++;
        if (*p == ']' || *p == '\0') break;
        out_arr[count++] = atoi(p);
        while (*p && *p != ',' && *p != ']') p++;
        if (*p == ',') p++;
    }
    return count;
}

static int parse_2d_int_array(const char *json_2d_start, int out_arr[MAX_DEADLOCK_PROCESSES][MAX_DEADLOCK_RESOURCES],
                              int max_rows, int max_cols, int *out_rows, int *out_cols) {
    if (!json_2d_start || !out_arr) return -1;
    const char *outer = strchr(json_2d_start, '[');
    if (!outer) return -1;
    outer++;

    int rows = 0;
    int cols = 0;

    while (*outer && *outer != ']' && rows < max_rows) {
        const char *inner = strchr(outer, '[');
        if (!inner) break;
        const char *inner_end = strchr(inner, ']');
        if (!inner_end) break;

        int row_vals[MAX_DEADLOCK_RESOURCES];
        int num_in_row = parse_int_array(inner, row_vals, max_cols);
        if (num_in_row > cols) cols = num_in_row;

        for (int j = 0; j < num_in_row && j < max_cols; j++) {
            out_arr[rows][j] = row_vals[j];
        }
        rows++;
        outer = inner_end + 1;
    }

    if (out_rows) *out_rows = rows;
    if (out_cols) *out_cols = cols;
    return (rows > 0) ? 0 : -1;
}

static int parse_deadlock_state(const char *body, deadlock_system_state_t *state) {
    if (!body || !state) return -1;
    memset(state, 0, sizeof(deadlock_system_state_t));

    /* Check for preset / example_id shortcut */
    const char *ex_key = strstr(body, "\"example_id\"");
    if (!ex_key) ex_key = strstr(body, "\"preset\"");
    if (ex_key) {
        char ex_id[64] = {0};
        const char *colon = strchr(ex_key, ':');
        if (colon) {
            const char *q1 = strchr(colon, '"');
            if (q1) {
                const char *q2 = strchr(q1 + 1, '"');
                if (q2 && (q2 - q1 - 1) < 63) {
                    size_t elen = q2 - q1 - 1;
                    memcpy(ex_id, q1 + 1, elen);
                    ex_id[elen] = '\0';
                    if (deadlock_get_example(ex_id, state) == 0) {
                        return 0;
                    }
                }
            }
        }
    }

    /* Parse Available */
    const char *avail_key = strstr(body, "\"available\"");
    if (!avail_key) return -1;
    int num_res = parse_int_array(avail_key, state->available, MAX_DEADLOCK_RESOURCES);
    state->num_resources = num_res;

    /* Parse Allocation */
    const char *alloc_key = strstr(body, "\"allocation\"");
    if (!alloc_key) return -1;
    int alloc_rows = 0, alloc_cols = 0;
    if (parse_2d_int_array(alloc_key, state->allocation, MAX_DEADLOCK_PROCESSES, MAX_DEADLOCK_RESOURCES, &alloc_rows, &alloc_cols) != 0) {
        return -1;
    }
    state->num_processes = alloc_rows;
    if (alloc_cols > state->num_resources) state->num_resources = alloc_cols;

    /* Parse Max Claim */
    const char *max_key = strstr(body, "\"max_claim\"");
    if (!max_key) max_key = strstr(body, "\"max\"");
    if (!max_key) return -1;
    int max_rows = 0, max_cols = 0;
    parse_2d_int_array(max_key, state->max_claim, MAX_DEADLOCK_PROCESSES, MAX_DEADLOCK_RESOURCES, &max_rows, &max_cols);

    /* Generate default process and resource names if empty */
    for (int i = 0; i < state->num_processes; i++) {
        snprintf(state->process_names[i], sizeof(state->process_names[i]), "P%d", i);
    }
    for (int j = 0; j < state->num_resources; j++) {
        snprintf(state->resource_names[j], sizeof(state->resource_names[j]), "R%d", j);
    }

    deadlock_calculate_need(state);
    return 0;
}

static void route_deadlock_banker(const char *body, http_response_t *res) {
    if (!body || strlen(body) < 10) {
        http_response_set_error(res, 400, "Missing or invalid deadlock state body");
        return;
    }

    deadlock_system_state_t state;
    if (parse_deadlock_state(body, &state) != 0) {
        http_response_set_error(res, 400, "Failed to parse available, allocation, or max_claim matrices");
        return;
    }

    char err_buf[128] = {0};
    if (deadlock_validate_state(&state, err_buf, sizeof(err_buf)) != 0) {
        http_response_set_error(res, 400, err_buf[0] ? err_buf : "Invalid state matrices");
        return;
    }

    bankers_result_t result;
    bankers_safety_analysis(&state, &result);

    json_builder_t jb;
    json_init(&jb, 8192);

    json_obj_start(&jb);
    json_add_bool(&jb, "is_safe", result.is_safe);
    json_add_string(&jb, "status_message", result.status_message);
    json_add_string(&jb, "explanation", result.explanation);
    json_add_int(&jb, "num_processes", state.num_processes);
    json_add_int(&jb, "num_resources", state.num_resources);

    /* Safe sequence */
    json_key(&jb, "safe_sequence");
    json_arr_start(&jb);
    for (int i = 0; i < result.sequence_length; i++) {
        json_arr_add_int(&jb, result.safe_sequence[i]);
    }
    json_arr_end(&jb);

    /* Step execution traces */
    json_key(&jb, "steps");
    json_arr_start(&jb);
    for (int s = 0; s < result.step_count; s++) {
        json_obj_start(&jb);
        json_add_int(&jb, "step", s);
        int p_idx = result.step_process[s];
        json_add_int(&jb, "process_idx", p_idx);
        json_add_string(&jb, "process_name", state.process_names[p_idx]);

        json_key(&jb, "work");
        json_arr_start(&jb);
        for (int j = 0; j < state.num_resources; j++) {
            json_arr_add_int(&jb, result.step_work[s][j]);
        }
        json_arr_end(&jb);
        json_obj_end(&jb);
    }
    json_arr_end(&jb);

    /* Need matrix */
    json_key(&jb, "need");
    json_arr_start(&jb);
    for (int i = 0; i < state.num_processes; i++) {
        json_arr_start(&jb);
        for (int j = 0; j < state.num_resources; j++) {
            json_arr_add_int(&jb, state.need[i][j]);
        }
        json_arr_end(&jb);
    }
    json_arr_end(&jb);

    /* Available vector */
    json_key(&jb, "available");
    json_arr_start(&jb);
    for (int j = 0; j < state.num_resources; j++) json_arr_add_int(&jb, state.available[j]);
    json_arr_end(&jb);

    json_add_uint64(&jb, "timestamp", (uint64_t)time(NULL));
    json_obj_end(&jb);

    http_response_set_json(res, 200, &jb);
    json_free(&jb);
}

static void route_deadlock_request(const char *body, http_response_t *res) {
    if (!body || strlen(body) < 10) {
        http_response_set_error(res, 400, "Missing request body");
        return;
    }

    deadlock_system_state_t state;
    if (parse_deadlock_state(body, &state) != 0) {
        http_response_set_error(res, 400, "Invalid state matrices");
        return;
    }

    int proc_idx = 0;
    const char *p_key = strstr(body, "\"process_idx\"");
    if (p_key) proc_key_val: {
        const char *c = strchr(p_key, ':');
        if (c) proc_idx = atoi(c + 1);
    }

    int req_vec[MAX_DEADLOCK_RESOURCES] = {0};
    const char *req_key = strstr(body, "\"request\"");
    if (req_key) {
        parse_int_array(req_key, req_vec, state.num_resources);
    }

    resource_request_result_t result;
    bankers_check_request(&state, proc_idx, req_vec, &result);

    json_builder_t jb;
    json_init(&jb, 4096);

    json_obj_start(&jb);
    json_add_int(&jb, "process_idx", proc_idx);
    json_add_bool(&jb, "can_grant", result.can_grant);
    json_add_bool(&jb, "would_be_safe", result.would_be_safe);
    json_add_string(&jb, "reason", result.reason);

    json_key(&jb, "resulting_safe_sequence");
    json_arr_start(&jb);
    for (int i = 0; i < result.resulting_state.sequence_length; i++) {
        json_arr_add_int(&jb, result.resulting_state.safe_sequence[i]);
    }
    json_arr_end(&jb);

    json_add_uint64(&jb, "timestamp", (uint64_t)time(NULL));
    json_obj_end(&jb);

    http_response_set_json(res, 200, &jb);
    json_free(&jb);
}

static void route_deadlock_detect(const char *body, http_response_t *res) {
    deadlock_system_state_t state;
    if (parse_deadlock_state(body, &state) != 0) {
        http_response_set_error(res, 400, "Invalid state matrices");
        return;
    }

    deadlock_detection_result_t det;
    deadlock_detect(&state, NULL, &det);

    json_builder_t jb;
    json_init(&jb, 4096);

    json_obj_start(&jb);
    json_add_bool(&jb, "is_deadlocked", det.is_deadlocked);
    json_add_int(&jb, "deadlocked_count", det.deadlocked_count);
    json_add_string(&jb, "message", det.message);
    json_add_string(&jb, "explanation", det.explanation);

    json_key(&jb, "deadlocked_processes");
    json_arr_start(&jb);
    for (int i = 0; i < det.deadlocked_count; i++) {
        json_arr_add_int(&jb, det.deadlocked_processes[i]);
    }
    json_arr_end(&jb);

    json_add_uint64(&jb, "timestamp", (uint64_t)time(NULL));
    json_obj_end(&jb);

    http_response_set_json(res, 200, &jb);
    json_free(&jb);
}

static void serialize_deadlock_example(json_builder_t *jb, const char *id, const char *title, const char *desc) {
    deadlock_system_state_t st;
    if (deadlock_get_example(id, &st) != 0) return;

    json_obj_start(jb);
    json_add_string(jb, "id", id);
    json_add_string(jb, "title", title);
    json_add_string(jb, "description", desc);
    json_add_int(jb, "num_processes", st.num_processes);
    json_add_int(jb, "num_resources", st.num_resources);

    /* Process names */
    json_key(jb, "processes");
    json_arr_start(jb);
    for (int i = 0; i < st.num_processes; i++) json_arr_add_string(jb, st.process_names[i]);
    json_arr_end(jb);

    /* Resource names */
    json_key(jb, "resources");
    json_arr_start(jb);
    for (int j = 0; j < st.num_resources; j++) json_arr_add_string(jb, st.resource_names[j]);
    json_arr_end(jb);

    /* Available */
    json_key(jb, "available");
    json_arr_start(jb);
    for (int j = 0; j < st.num_resources; j++) json_arr_add_int(jb, st.available[j]);
    json_arr_end(jb);

    /* Allocation */
    json_key(jb, "allocation");
    json_arr_start(jb);
    for (int i = 0; i < st.num_processes; i++) {
        json_arr_start(jb);
        for (int j = 0; j < st.num_resources; j++) json_arr_add_int(jb, st.allocation[i][j]);
        json_arr_end(jb);
    }
    json_arr_end(jb);

    /* Max Claim */
    json_key(jb, "max_claim");
    json_arr_start(jb);
    for (int i = 0; i < st.num_processes; i++) {
        json_arr_start(jb);
        for (int j = 0; j < st.num_resources; j++) json_arr_add_int(jb, st.max_claim[i][j]);
        json_arr_end(jb);
    }
    json_arr_end(jb);

    json_obj_end(jb);
}

static void route_deadlock_examples(http_response_t *res) {
    json_builder_t jb;
    json_init(&jb, 8192);

    json_obj_start(&jb);
    json_key(&jb, "examples");
    json_arr_start(&jb);

    serialize_deadlock_example(&jb, "silberschatz_safe",
        "Silberschatz Textbook Safe State",
        "Classic 5-process, 3-resource system with available resources [3, 3, 2] producing safe sequence P1 -> P3 -> P4 -> P0 -> P2.");

    serialize_deadlock_example(&jb, "unsafe_state",
        "Unsafe State (Deadlock Avoidance Warning)",
        "Available resources are insufficient to guarantee maximum claim satisfaction; illustrates why unsafe != deadlock.");

    serialize_deadlock_example(&jb, "deadlock_cycle",
        "Active Deadlock Cycle (Circular Wait)",
        "Processes A and B hold mutually desired mutexes with 0 available resources, creating an unresolved circular wait.");

    json_arr_end(&jb);
    json_obj_end(&jb);

    http_response_set_json(res, 200, &jb);
    json_free(&jb);
}

/* ==================== Phase 3: OS Log Analysis Routes ==================== */

static void route_log_analyze(const char *body, http_response_t *res) {
    if (!body || strlen(body) < 5) {
        http_response_set_error(res, 400, "Missing log payload");
        return;
    }

    log_filter_t filter;
    memset(&filter, 0, sizeof(filter));
    filter.category_filter = -1;

    /* Parse filter options if present */
    const char *kw = strstr(body, "\"keyword\"");
    if (kw) {
        const char *q1 = strchr(kw + 9, '"');
        if (q1) {
            const char *q2 = strchr(q1 + 1, '"');
            if (q2) {
                size_t klen = q2 - q1 - 1;
                if (klen >= sizeof(filter.keyword)) klen = sizeof(filter.keyword) - 1;
                memcpy(filter.keyword, q1 + 1, klen);
                filter.keyword[klen] = '\0';
            }
        }
    }

    const char *sev_str = strstr(body, "\"min_severity\"");
    if (sev_str) {
        const char *q1 = strchr(sev_str + 14, '"');
        if (q1) {
            char sbuf[32] = {0};
            sscanf(q1 + 1, "%31[^\"]", sbuf);
            filter.min_severity = log_severity_from_str(sbuf);
        }
    }

    const char *cat_ptr = strstr(body, "\"category\"");
    if (cat_ptr) {
        const char *c = strchr(cat_ptr, ':');
        if (c) filter.category_filter = atoi(c + 1);
    }

    /* Heap allocate report structure (~250 KB) to prevent stack overflow */
    log_analysis_report_t *report = (log_analysis_report_t *)calloc(1, sizeof(log_analysis_report_t));
    if (!report) {
        http_response_set_error(res, 500, "Memory allocation failure for log analysis");
        return;
    }

    /* Extract content string */
    const char *content_key = strstr(body, "\"content\"");
    if (!content_key) content_key = strstr(body, "\"log_text\"");
    if (content_key) {
        const char *colon = strchr(content_key, ':');
        const char *q1 = colon ? strchr(colon, '"') : NULL;
        if (q1) {
            /* Decode JSON string or pass content directly */
            q1++;
            size_t clen = strlen(q1);
            char *raw_content = (char *)malloc(clen + 1);
            if (raw_content) {
                size_t w = 0;
                for (size_t r = 0; r < clen; r++) {
                    if (q1[r] == '\\' && q1[r+1] == 'n') {
                        raw_content[w++] = '\n';
                        r++;
                    } else if (q1[r] == '\\' && q1[r+1] == 'r') {
                        r++;
                    } else if (q1[r] == '\\' && q1[r+1] == '"') {
                        raw_content[w++] = '"';
                        r++;
                    } else if (q1[r] == '"' && (r == clen - 1 || q1[r+1] == '}' || q1[r+1] == ',')) {
                        break;
                    } else {
                        raw_content[w++] = q1[r];
                    }
                }
                raw_content[w] = '\0';
                log_analyze_content(raw_content, &filter, report);
                free(raw_content);
            }
        }
    } else {
        /* Check if file_path is provided */
        const char *fp_key = strstr(body, "\"file_path\"");
        if (fp_key) {
            char path_buf[256] = {0};
            const char *q1 = strchr(fp_key + 11, '"');
            if (q1) {
                sscanf(q1 + 1, "%255[^\"]", path_buf);
                int r = log_analyze_file(path_buf, &filter, report);
                if (r == -4) {
                    free(report);
                    http_response_set_error(res, 403, "Access Denied: Log file analysis is restricted to safe directory paths and non-sensitive log formats");
                    return;
                } else if (r == -5) {
                    free(report);
                    http_response_set_error(res, 413, "Payload Too Large: Log file exceeds 5MB size limit");
                    return;
                } else if (r != 0) {
                    free(report);
                    http_response_set_error(res, 404, "Log file not found or could not be opened");
                    return;
                }
            }
        }
    }

    json_builder_t jb;
    json_init(&jb, 65536);

    json_obj_start(&jb);
    json_add_int(&jb, "total_lines_read", report->total_lines_read);
    json_add_int(&jb, "parsed_entries_count", report->parsed_entries_count);
    json_add_int(&jb, "unrecognized_lines_count", report->unrecognized_lines_count);
    json_add_int(&jb, "error_count", report->error_count);
    json_add_int(&jb, "warning_count", report->warning_count);
    json_add_int(&jb, "info_count", report->info_count);
    json_add_int(&jb, "debug_count", report->debug_count);
    json_add_bool(&jb, "burst_detected", report->burst_detected);
    json_add_string(&jb, "burst_description", report->burst_description);

    /* Category counts object */
    json_key(&jb, "category_counts");
    json_obj_start(&jb);
    json_add_int(&jb, "general", report->category_counts[LOG_CAT_GENERAL]);
    json_add_int(&jb, "memory_oom", report->category_counts[LOG_CAT_MEMORY_OOM]);
    json_add_int(&jb, "kernel_panic", report->category_counts[LOG_CAT_KERNEL_PANIC]);
    json_add_int(&jb, "storage_disk", report->category_counts[LOG_CAT_STORAGE_DISK]);
    json_add_int(&jb, "auth_security", report->category_counts[LOG_CAT_AUTH_SECURITY]);
    json_add_int(&jb, "network", report->category_counts[LOG_CAT_NETWORK]);
    json_add_int(&jb, "service_crash", report->category_counts[LOG_CAT_SERVICE_CRASH]);
    json_obj_end(&jb);

    /* Recurring patterns */
    json_key(&jb, "recurring_patterns");
    json_arr_start(&jb);
    for (int p = 0; p < report->recurring_pattern_count; p++) {
        json_obj_start(&jb);
        json_add_string(&jb, "pattern", report->recurring_patterns[p].pattern);
        json_add_int(&jb, "count", report->recurring_patterns[p].count);
        json_add_string(&jb, "severity", log_severity_to_str(report->recurring_patterns[p].severity));
        json_obj_end(&jb);
    }
    json_arr_end(&jb);

    /* Matched Entries array */
    json_key(&jb, "entries");
    json_arr_start(&jb);
    for (int e = 0; e < report->entry_count; e++) {
        log_entry_t *ent = &report->entries[e];
        json_obj_start(&jb);
        json_add_int(&jb, "line", ent->line_number);
        json_add_string(&jb, "timestamp", ent->timestamp_str);
        json_add_string(&jb, "source", ent->source);
        json_add_string(&jb, "severity", ent->severity_name);
        json_add_string(&jb, "category", ent->category_name);
        json_add_string(&jb, "message", ent->message);
        json_add_bool(&jb, "is_supported_format", ent->is_supported_format);
        json_obj_end(&jb);
    }
    json_arr_end(&jb);

    json_add_uint64(&jb, "timestamp", (uint64_t)time(NULL));
    json_obj_end(&jb);

    free(report);
    http_response_set_json(res, 200, &jb);
    json_free(&jb);
}

static void route_log_samples(const char *query, http_response_t *res) {
    char sample_id[64] = "linux_syslog";
    if (query) {
        const char *id_ptr = strstr(query, "id=");
        if (id_ptr) sscanf(id_ptr + 3, "%63[^&]", sample_id);
    }

    char buf[4096] = {0};
    if (log_get_sample(sample_id, buf, sizeof(buf)) != 0) {
        log_get_sample("linux_syslog", buf, sizeof(buf));
    }

    json_builder_t jb;
    json_init(&jb, 8192);

    json_obj_start(&jb);
    json_add_string(&jb, "sample_id", sample_id);
    json_add_string(&jb, "content", buf);
    json_add_uint64(&jb, "timestamp", (uint64_t)time(NULL));
    json_obj_end(&jb);

    http_response_set_json(res, 200, &jb);
    json_free(&jb);
}

/* ==================== Phase 3: Advisory Resource Allocation Routes ==================== */

static void route_advisory_recommendations(http_response_t *res) {
    resource_advisory_report_t *report = (resource_advisory_report_t *)calloc(1, sizeof(resource_advisory_report_t));
    if (!report) {
        http_response_set_error(res, 500, "Memory allocation error");
        return;
    }

    resource_advisory_generate(report);

    json_builder_t jb;
    json_init(&jb, 16384);

    json_obj_start(&jb);

    /* Overview block */
    json_key(&jb, "overview");
    json_obj_start(&jb);
    json_add_string(&jb, "cpu_pressure", report->overview.cpu_pressure);
    json_add_string(&jb, "memory_pressure", report->overview.memory_pressure);
    json_add_string(&jb, "process_pressure", report->overview.process_pressure);
    json_add_string(&jb, "paging_pressure", report->overview.paging_pressure);
    json_add_string(&jb, "overall_health", report->overview.overall_health);
    json_add_int(&jb, "total_recommendations", report->overview.total_recommendations);
    json_add_int(&jb, "warning_count", report->overview.warning_count);
    json_add_int(&jb, "critical_count", report->overview.critical_count);
    json_obj_end(&jb);

    /* Advisory items */
    json_key(&jb, "items");
    json_arr_start(&jb);
    for (int i = 0; i < report->item_count; i++) {
        advisory_item_t *item = &report->items[i];
        json_obj_start(&jb);
        json_add_string(&jb, "id", item->id);
        json_add_int(&jb, "category_id", (int)item->category);
        json_add_string(&jb, "category", item->category_name);
        json_add_int(&jb, "severity_id", (int)item->severity);
        json_add_string(&jb, "severity", item->severity_name);
        json_add_string(&jb, "title", item->title);
        json_add_string(&jb, "rationale", item->rationale);
        json_add_string(&jb, "recommended_action", item->recommended_action);
        json_add_int(&jb, "confidence_pct", item->confidence_pct);
        json_add_bool(&jb, "is_pure_telemetry", item->is_pure_telemetry);
        json_add_string(&jb, "metric_name", item->metric_name);
        json_add_string(&jb, "measured_value", item->measured_value);
        json_add_string(&jb, "threshold_value", item->threshold_value);
        json_obj_end(&jb);
    }
    json_arr_end(&jb);

    json_add_string(&jb, "disclaimer", report->disclaimer);
    json_add_uint64(&jb, "timestamp", (uint64_t)time(NULL));
    json_obj_end(&jb);

    free(report);
    http_response_set_json(res, 200, &jb);
    json_free(&jb);
}

/* ==================== Master Request Dispatcher ==================== */

void handle_api_request(const char *method, const char *path, const char *query,
                        const char *req_body, http_response_t *res) {
    if (strcmp(method, "OPTIONS") == 0) {
        res->status_code = 204;
        res->status_text = "No Content";
        res->content_type = "text/plain";
        res->body = NULL;
        res->body_len = 0;
        return;
    }

    if (strcmp(method, "GET") == 0) {
        if (strcmp(path, "/api/health") == 0) {
            route_health(res);
        } else if (strcmp(path, "/api/system/overview") == 0) {
            route_system_overview(res);
        } else if (strcmp(path, "/api/system/cpu") == 0) {
            route_system_cpu(res);
        } else if (strcmp(path, "/api/system/memory") == 0) {
            route_system_memory(res);
        } else if (strcmp(path, "/api/processes") == 0) {
            route_processes(res);
        } else if (strncmp(path, "/api/processes/", 15) == 0) {
            const char *pid_str = path + 15;
            route_process_detail(pid_str, res);
        } else if (strcmp(path, "/api/scheduling/examples") == 0) {
            route_scheduling_examples(res);
        } else if (strcmp(path, "/api/memory/forecast") == 0) {
            route_memory_forecast(query, res);
        } else if (strcmp(path, "/api/memory/history") == 0) {
            route_memory_history(res);
        } else if (strcmp(path, "/api/page-replacement/examples") == 0) {
            route_page_examples(res);
        } else if (strcmp(path, "/api/deadlock/examples") == 0) {
            route_deadlock_examples(res);
        } else if (strcmp(path, "/api/logs/samples") == 0) {
            route_log_samples(query, res);
        } else if (strcmp(path, "/api/advisory/recommendations") == 0) {
            route_advisory_recommendations(res);
        } else {
            http_response_set_error(res, 404, "Endpoint not found");
        }
        return;
    }

    if (strcmp(method, "POST") == 0) {
        if (strcmp(path, "/api/scheduling/simulate") == 0) {
            route_scheduling_simulate(req_body, res);
        } else if (strcmp(path, "/api/page-replacement/simulate") == 0) {
            route_page_simulate(req_body, res);
        } else if (strcmp(path, "/api/deadlock/banker") == 0) {
            route_deadlock_banker(req_body, res);
        } else if (strcmp(path, "/api/deadlock/request") == 0) {
            route_deadlock_request(req_body, res);
        } else if (strcmp(path, "/api/deadlock/detect") == 0) {
            route_deadlock_detect(req_body, res);
        } else if (strcmp(path, "/api/logs/analyze") == 0) {
            route_log_analyze(req_body, res);
        } else {
            http_response_set_error(res, 404, "Endpoint not found for POST method");
        }
        return;
    }

    http_response_set_error(res, 405, "Method Not Allowed");
}

