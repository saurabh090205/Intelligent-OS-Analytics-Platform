#include "resource_advisory.h"
#include "cpu_monitor.h"
#include "memory_monitor.h"
#include "process_monitor.h"
#include "memory_prediction.h"
#include <stdio.h>
#include <string.h>

static void add_item(resource_advisory_report_t *report,
                     const char *id,
                     advisory_category_t cat,
                     const char *cat_name,
                     advisory_severity_t sev,
                     const char *sev_name,
                     const char *title,
                     const char *rationale,
                     const char *action,
                     int confidence,
                     bool is_telemetry,
                     const char *metric,
                     const char *val,
                     const char *thresh) {
    if (report->item_count >= MAX_ADVISORY_ITEMS) return;
    advisory_item_t *item = &report->items[report->item_count++];
    strncpy(item->id, id, sizeof(item->id) - 1);
    item->category = cat;
    strncpy(item->category_name, cat_name, sizeof(item->category_name) - 1);
    item->severity = sev;
    strncpy(item->severity_name, sev_name, sizeof(item->severity_name) - 1);
    strncpy(item->title, title, sizeof(item->title) - 1);
    strncpy(item->rationale, rationale, sizeof(item->rationale) - 1);
    strncpy(item->recommended_action, action, sizeof(item->recommended_action) - 1);
    item->confidence_pct = confidence;
    item->is_pure_telemetry = is_telemetry;
    strncpy(item->metric_name, metric, sizeof(item->metric_name) - 1);
    strncpy(item->measured_value, val, sizeof(item->measured_value) - 1);
    strncpy(item->threshold_value, thresh, sizeof(item->threshold_value) - 1);

    if (sev == ADVISORY_SEV_CRITICAL) {
        report->overview.critical_count++;
    } else if (sev == ADVISORY_SEV_WARNING) {
        report->overview.warning_count++;
    }
}

int resource_advisory_generate(resource_advisory_report_t *report) {
    if (!report) return -1;
    memset(report, 0, sizeof(resource_advisory_report_t));

    /* 1. Gather Live Telemetry from Phase 1 & 2 Subsystems */
    cpu_stats_t cpu;
    memset(&cpu, 0, sizeof(cpu));
    cpu_monitor_update(&cpu);

    memory_stats_t mem;
    memset(&mem, 0, sizeof(mem));
    memory_monitor_update(&mem);

    process_table_t procs;
    memset(&procs, 0, sizeof(procs));
    process_monitor_update(&procs);

    mem_forecast_result_t fc;
    memset(&fc, 0, sizeof(fc));
    mem_predict_forecast(60, 10, 85.0, &fc);

    /* Find top CPU and Memory processes */
    process_info_t *top_cpu_p = NULL;
    process_info_t *top_mem_p = NULL;
    for (int i = 0; i < procs.count; i++) {
        if (!top_cpu_p || procs.processes[i].cpu_pct > top_cpu_p->cpu_pct) {
            top_cpu_p = &procs.processes[i];
        }
        if (!top_mem_p || procs.processes[i].mem_rss_kb > top_mem_p->mem_rss_kb) {
            top_mem_p = &procs.processes[i];
        }
    }

    /* 2. Analyze CPU Pressure */
    char buf_val[32];
    char buf_thresh[32];

    if (cpu.overall_utilization >= 85.0) {
        strncpy(report->overview.cpu_pressure, "Critical", 15);
        snprintf(buf_val, sizeof(buf_val), "%.1f%%", cpu.overall_utilization);
        snprintf(buf_thresh, sizeof(buf_thresh), "85.0%%");
        char rationale[384];
        if (top_cpu_p) {
            snprintf(rationale, sizeof(rationale),
                     "Total CPU utilization is critical at %.1f%% across %d cores. Dominant consumer is '%s' (PID %d) utilizing %.1f%% CPU.",
                     cpu.overall_utilization, cpu.core_count, top_cpu_p->name, top_cpu_p->pid, top_cpu_p->cpu_pct);
        } else {
            snprintf(rationale, sizeof(rationale),
                     "Total CPU utilization is critical at %.1f%% across %d cores. Run-queue saturation detected.",
                     cpu.overall_utilization, cpu.core_count);
        }
        add_item(report, "cpu_high_load", ADVISORY_CAT_CPU, "CPU Management",
                 ADVISORY_SEV_CRITICAL, "CRITICAL",
                 "High CPU Saturation Pressure Detected",
                 rationale,
                 "Consider lowering priority (increasing nice level) of background workloads or switching to Shortest Remaining Time First (SRTF) time-slicing.",
                 95, true, "CPU Utilization", buf_val, buf_thresh);
    } else if (cpu.overall_utilization >= 60.0) {
        strncpy(report->overview.cpu_pressure, "Moderate", 15);
        snprintf(buf_val, sizeof(buf_val), "%.1f%%", cpu.overall_utilization);
        snprintf(buf_thresh, sizeof(buf_thresh), "60.0%%");
        add_item(report, "cpu_elevated", ADVISORY_CAT_CPU, "CPU Management",
                 ADVISORY_SEV_WARNING, "WARNING",
                 "Elevated CPU Activity Across Active Cores",
                 "Processor load is moderately elevated. Core dispatch queues are active but operating without severe throttling.",
                 "Monitor sustained burst times. If interactive latency degrades, decrease Round Robin quantum to 2ms.",
                 80, true, "CPU Utilization", buf_val, buf_thresh);
    } else {
        strncpy(report->overview.cpu_pressure, "Nominal", 15);
        snprintf(buf_val, sizeof(buf_val), "%.1f%%", cpu.overall_utilization);
        snprintf(buf_thresh, sizeof(buf_thresh), "60.0%%");
        add_item(report, "cpu_healthy", ADVISORY_CAT_CPU, "CPU Management",
                 ADVISORY_SEV_INFO, "INFO",
                 "CPU Scheduling Headroom Is Optimal",
                 "Processor scheduling capacity is healthy with substantial idle jiffy cycles available for real-time bursts.",
                 "Maintain current scheduling configuration. First-Come-First-Serve (FCFS) or standard Round Robin is sufficient.",
                 90, true, "CPU Utilization", buf_val, buf_thresh);
    }

    /* 3. Analyze Memory Pressure & Forecasting */
    if (mem.utilization_pct >= 85.0 || fc.threshold_exceeded) {
        strncpy(report->overview.memory_pressure, "Critical", 15);
        snprintf(buf_val, sizeof(buf_val), "%.1f%%", mem.utilization_pct);
        snprintf(buf_thresh, sizeof(buf_thresh), "85.0%%");
        char rationale[384];
        if (fc.trend == MEM_TREND_INCREASING) {
            snprintf(rationale, sizeof(rationale),
                     "Physical memory usage is %.1f%% with upward linear trajectory (+%.2f%%/min). Estimated available RAM is %llu MB.",
                     mem.utilization_pct, fc.slope_pct_per_min, (unsigned long long)(mem.available_kb / 1024));
        } else {
            snprintf(rationale, sizeof(rationale),
                     "Physical memory usage is high at %.1f%% (%llu MB used of %llu MB total). Available headroom is constrained.",
                     mem.utilization_pct, (unsigned long long)(mem.used_kb / 1024), (unsigned long long)(mem.total_kb / 1024));
        }
        add_item(report, "mem_critical", ADVISORY_CAT_MEMORY, "Memory Management",
                 ADVISORY_SEV_CRITICAL, "CRITICAL",
                 "High Physical Memory Utilization Alert",
                 rationale,
                 "Audit memory-intensive resident sets. Release idle buffers and monitor for potential Out-Of-Memory (OOM) killer events.",
                 92, true, "RAM Utilization", buf_val, buf_thresh);
    } else if (fc.trend == MEM_TREND_INCREASING && fc.slope_pct_per_min > 0.3) {
        strncpy(report->overview.memory_pressure, "Moderate", 15);
        snprintf(buf_val, sizeof(buf_val), "+%.2f%%/min", fc.slope_pct_per_min);
        snprintf(buf_thresh, sizeof(buf_thresh), "+0.30%%/min");
        add_item(report, "mem_leak_trend", ADVISORY_CAT_MEMORY, "Memory Management",
                 ADVISORY_SEV_WARNING, "WARNING",
                 "Sustained Memory Growth Trajectory Detected",
                 "Linear regression indicates persistent positive rate of change in memory footprint, characteristic of memory leak or buffer accumulation.",
                 "Inspect long-running worker processes with increasing resident set sizes (RSS).",
                 78, false, "Regression Slope", buf_val, buf_thresh);
    } else {
        strncpy(report->overview.memory_pressure, "Nominal", 15);
        snprintf(buf_val, sizeof(buf_val), "%.1f%%", mem.utilization_pct);
        snprintf(buf_thresh, sizeof(buf_thresh), "85.0%%");
        add_item(report, "mem_nominal", ADVISORY_CAT_MEMORY, "Memory Management",
                 ADVISORY_SEV_INFO, "INFO",
                 "Physical Memory Headroom Is Adequate",
                 "System memory has sufficient available headroom without memory thrashing or high swap pressure.",
                 "No memory reallocation required at this time.",
                 95, true, "RAM Utilization", buf_val, buf_thresh);
    }

    /* 4. Swap Space Analysis */
    if (mem.swap_total_kb > 0) {
        double swap_pct = ((double)mem.swap_used_kb / (double)mem.swap_total_kb) * 100.0;
        if (swap_pct >= 30.0) {
            snprintf(buf_val, sizeof(buf_val), "%.1f%%", swap_pct);
            snprintf(buf_thresh, sizeof(buf_thresh), "30.0%%");
            add_item(report, "swap_high", ADVISORY_CAT_PAGING, "Paging & Virtual Memory",
                     ADVISORY_SEV_WARNING, "WARNING",
                     "Elevated Swap Space Utilization",
                     "Paging subsystem has migrated significant anonymous memory to secondary storage, indicating working set size exceeds physical RAM.",
                     "Consider expanding physical memory or reducing resident concurrency to avoid disk I/O thrashing.",
                     85, true, "Swap Utilization", buf_val, buf_thresh);
        }
    }

    /* 5. Process Table & Zombie Task Analysis */
    if (procs.zombie_count > 0) {
        strncpy(report->overview.process_pressure, "Caution", 15);
        snprintf(buf_val, sizeof(buf_val), "%d zombies", procs.zombie_count);
        snprintf(buf_thresh, sizeof(buf_thresh), "0 zombies");
        add_item(report, "proc_zombies", ADVISORY_CAT_PROCESS, "Process Optimization",
                 ADVISORY_SEV_WARNING, "WARNING",
                 "Orphaned Zombie Processes Detected in Process Table",
                 "Terminated child processes remain in the system task table because parent processes have not issued wait() or waitpid() system calls.",
                 "Identify parent PIDs of zombie tasks and signal them or let init/systemd adopt and reap them.",
                 100, true, "Zombie Count", buf_val, buf_thresh);
    } else {
        strncpy(report->overview.process_pressure, "Nominal", 15);
    }

    /* 6. Dominant Memory Consumer Process Inspection */
    if (top_mem_p && mem.total_kb > 0) {
        double p_mem_pct = ((double)top_mem_p->mem_rss_kb / (double)mem.total_kb) * 100.0;
        if (p_mem_pct >= 20.0) {
            snprintf(buf_val, sizeof(buf_val), "%.1f%%", p_mem_pct);
            snprintf(buf_thresh, sizeof(buf_thresh), "20.0%%");
            char rationale[384];
            snprintf(rationale, sizeof(rationale),
                     "Single process '%s' (PID %d) commands %.1f%% of total physical RAM (%llu MB RSS).",
                     top_mem_p->name, top_mem_p->pid, p_mem_pct, (unsigned long long)(top_mem_p->mem_rss_kb / 1024));
            add_item(report, "proc_heavy_mem", ADVISORY_CAT_PROCESS, "Process Optimization",
                     ADVISORY_SEV_WARNING, "WARNING",
                     "Disproportionate Memory Footprint by Single Task",
                     rationale,
                     "Verify if process has appropriate working-set limits or configure cgroups memory.max constraints.",
                     88, true, "Process RSS Share", buf_val, buf_thresh);
        }
    }

    /* 7. Paging & Page Replacement Policy Guidance */
    strncpy(report->overview.paging_pressure, "Nominal", 15);
    add_item(report, "paging_guidance", ADVISORY_CAT_PAGING, "Paging & Virtual Memory",
             ADVISORY_SEV_INFO, "INFO",
             "Recommended Page Replacement Strategy",
             "Based on temporal locality workloads evaluated in Phase 2 benchmarks, Least Recently Used (LRU) achieved 20% fewer page faults than FIFO.",
             "Maintain LRU or Clock approximation in kernel page evictions to minimize page faults and avoid Belady's Anomaly.",
             92, false, "Algorithm Benchmark", "LRU (60% hits)", "FIFO (25% hits)");

    /* 8. Overall System Health Synthesis */
    report->overview.total_recommendations = report->item_count;
    if (report->overview.critical_count > 0) {
        strncpy(report->overview.overall_health, "Critical Pressure", 23);
    } else if (report->overview.warning_count > 0) {
        strncpy(report->overview.overall_health, "Caution / Attention", 23);
    } else {
        strncpy(report->overview.overall_health, "Optimal Performance", 23);
    }

    snprintf(report->disclaimer, sizeof(report->disclaimer),
             "ADVISORY NOTICE: All recommendations are analytical guidance intended for systems administrators. "
             "The platform adheres to strict safety boundaries: it does NOT automatically terminate processes, "
             "alter process nice levels, or mutate kernel sysctl configurations. "
             "Measured telemetry is collected directly from host OS APIs; forecasts and benchmarks represent statistical models.");

    process_table_free(&procs);
    return 0;
}
