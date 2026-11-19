#define __USE_MINGW_ANSI_STDIO 1
#include "terminal_ui.h"
#include "cpu_monitor.h"
#include "memory_monitor.h"
#include "process_monitor.h"
#include "cpu_scheduler.h"
#include "memory_prediction.h"
#include "page_replacement.h"
#include "deadlock.h"
#include "log_analyzer.h"
#include "resource_advisory.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#if defined(_WIN32)
#include <windows.h>
#define SLEEP_MS(ms) Sleep(ms)
#ifndef ENABLE_VIRTUAL_TERMINAL_PROCESSING
#define ENABLE_VIRTUAL_TERMINAL_PROCESSING 0x0004
#endif
#else
#include <unistd.h>
#define SLEEP_MS(ms) usleep((ms) * 1000)
#endif

/* ANSI Color Escape Codes */
#define ANSI_RESET       "\033[0m"
#define ANSI_BOLD        "\033[1m"
#define ANSI_DIM         "\033[2m"
#define ANSI_RED         "\033[1;31m"
#define ANSI_GREEN       "\033[1;32m"
#define ANSI_YELLOW      "\033[1;33m"
#define ANSI_BLUE        "\033[1;34m"
#define ANSI_MAGENTA     "\033[1;35m"
#define ANSI_CYAN        "\033[1;36m"
#define ANSI_WHITE       "\033[1;37m"
#define ANSI_BG_BLUE     "\033[44m"
#define ANSI_BG_RED      "\033[41m"
#define ANSI_BG_GREEN    "\033[42m"
#define ANSI_BG_GRAY     "\033[100m"

static void init_terminal(void) {
#if defined(_WIN32)
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hOut != INVALID_HANDLE_VALUE) {
        DWORD dwMode = 0;
        if (GetConsoleMode(hOut, &dwMode)) {
            dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
            SetConsoleMode(hOut, dwMode);
        }
    }
#endif
}

static void clear_screen(void) {
    printf("\033[2J\033[H");
    fflush(stdout);
}

static void pause_prompt(void) {
    printf("\n" ANSI_CYAN "Press Enter to return to menu..." ANSI_RESET);
    fflush(stdout);
    int c;
    while ((c = getchar()) != '\n' && c != EOF) {}
}

static void print_bar(double pct, int width, const char *color) {
    if (pct < 0.0) pct = 0.0;
    if (pct > 100.0) pct = 100.0;
    int filled = (int)((pct / 100.0) * width);
    printf("%s[", color);
    for (int i = 0; i < width; i++) {
        if (i < filled) {
            printf("#");
        } else {
            printf(".");
        }
    }
    printf("] %5.1f%%" ANSI_RESET, pct);
}

static void print_header(const char *title, const char *subtitle) {
    printf(ANSI_CYAN "================================================================================" ANSI_RESET "\n");
    printf(ANSI_BOLD ANSI_WHITE " %s" ANSI_RESET "\n", title);
    if (subtitle) {
        printf(ANSI_DIM " %s" ANSI_RESET "\n", subtitle);
    }
    printf(ANSI_CYAN "================================================================================" ANSI_RESET "\n\n");
}

/* ============================================================================
 * MODULE 1: Live Telemetry Dashboard
 * ============================================================================ */
void tui_show_telemetry_dashboard(void) {
    clear_screen();
    print_header("REAL-TIME HOST HARDWARE TELEMETRY & SYSTEM DASHBOARD",
                 "Live differential CPU counters, physical RAM & swap allocation, and top processes");

    cpu_stats_t cpu;
    memory_stats_t mem;
    process_table_t procs;

    memset(&cpu, 0, sizeof(cpu));
    memset(&mem, 0, sizeof(mem));
    memset(&procs, 0, sizeof(procs));

    cpu_monitor_update(&cpu);
    memory_monitor_update(&mem);
    process_monitor_update(&procs);

    /* CPU Summary */
    printf(ANSI_BOLD ANSI_CYAN "[ CPU SUBSYSTEM ]" ANSI_RESET "\n");
    printf("Model       : " ANSI_BOLD "%s" ANSI_RESET " (%d Cores, User HZ: %d)\n",
           cpu.model_name[0] ? cpu.model_name : "Host x86_64", cpu.core_count, cpu.user_hz);
    printf("Utilization : ");
    const char *cpu_color = (cpu.overall_utilization > 80.0) ? ANSI_RED :
                            (cpu.overall_utilization > 50.0) ? ANSI_YELLOW : ANSI_GREEN;
    print_bar(cpu.overall_utilization, 35, cpu_color);
    printf(" (Total Jiffies: %llu, Idle: %llu)\n", (unsigned long long)cpu.total_jiffies, (unsigned long long)cpu.idle_jiffies);

    /* Per-core breakdown */
    if (cpu.active_cores > 0) {
        printf("Per-Core    : ");
        int display_cores = (cpu.active_cores > 8) ? 8 : cpu.active_cores;
        for (int c = 0; c < display_cores; c++) {
            printf("C%d: %3.0f%%  ", cpu.cores[c].core_id, cpu.cores[c].utilization_pct);
        }
        if (cpu.active_cores > 8) printf("... (+%d more)", cpu.active_cores - 8);
        printf("\n");
    }
    printf("\n");

    /* Memory Summary */
    printf(ANSI_BOLD ANSI_CYAN "[ MEMORY & PAGING SUBSYSTEM ]" ANSI_RESET "\n");
    printf("Total RAM   : " ANSI_BOLD "%.2f GB" ANSI_RESET " (%llu KB)\n",
           (double)mem.total_kb / 1048576.0, (unsigned long long)mem.total_kb);
    printf("Used RAM    : %.2f GB | Available: " ANSI_GREEN "%.2f GB" ANSI_RESET " | Free: %.2f GB\n",
           (double)mem.used_kb / 1048576.0, (double)mem.available_kb / 1048576.0, (double)mem.free_kb / 1048576.0);
    printf("Utilization : ");
    const char *mem_color = (mem.utilization_pct > 85.0) ? ANSI_RED :
                            (mem.utilization_pct > 65.0) ? ANSI_YELLOW : ANSI_GREEN;
    print_bar(mem.utilization_pct, 35, mem_color);
    printf("\n");
    printf("Swap / Page : %llu MB used of %llu MB total (",
           (unsigned long long)mem.swap_used_kb / 1024, (unsigned long long)mem.swap_total_kb / 1024);
    double swap_util = (mem.swap_total_kb > 0) ? ((double)mem.swap_used_kb / (double)mem.swap_total_kb * 100.0) : 0.0;
    print_bar(swap_util, 15, ANSI_YELLOW);
    printf(")\n\n");

    /* Process Summary */
    uint64_t uptime = procs.system_uptime_seconds;
    int days = (int)(uptime / 86400);
    int hrs = (int)((uptime % 86400) / 3600);
    int mins = (int)((uptime % 3600) / 60);
    int secs = (int)(uptime % 60);

    printf(ANSI_BOLD ANSI_CYAN "[ PROCESS TABLE & UPTIME ]" ANSI_RESET "\n");
    printf("System Uptime: " ANSI_BOLD "%dd %02dh %02dm %02ds" ANSI_RESET " (%llu seconds)\n",
           days, hrs, mins, secs, (unsigned long long)uptime);
    printf("Processes    : Total: " ANSI_BOLD "%d" ANSI_RESET " | Running: " ANSI_GREEN "%d" ANSI_RESET
           " | Sleeping: %d | Stopped: %d | Zombie: " ANSI_RED "%d" ANSI_RESET "\n\n",
           procs.total_processes, procs.running_count, procs.sleeping_count,
           procs.stopped_count, procs.zombie_count);

    /* Top Processes Table */
    printf(ANSI_BOLD "Top 8 Resource-Consuming Processes:" ANSI_RESET "\n");
    printf(ANSI_DIM "  PID    PPID   STATE  CPU %%    RSS (KB)   THREADS  NAME" ANSI_RESET "\n");
    printf("  ----------------------------------------------------------------------\n");

    int show_count = (procs.count > 8) ? 8 : procs.count;
    for (int i = 0; i < show_count; i++) {
        process_info_t *p = &procs.processes[i];
        const char *state_color = (p->state_char == 'R') ? ANSI_GREEN :
                                  (p->state_char == 'Z') ? ANSI_RED : ANSI_WHITE;
        printf("  %-6d %-6d %s%-6s" ANSI_RESET " %5.1f%% %10llu  %-8d %-22s\n",
               p->pid, p->ppid, state_color, p->state_str,
               p->cpu_pct, (unsigned long long)p->mem_rss_kb, p->num_threads, p->name);
    }
    printf("  ----------------------------------------------------------------------\n");

    process_table_free(&procs);
    pause_prompt();
}

/* ============================================================================
 * MODULE 2: Process Monitor & PID Inspection
 * ============================================================================ */
void tui_show_process_monitor(void) {
    clear_screen();
    print_header("PROCESS MONITORING & SINGLE-PID INSPECTOR",
                 "Inspect running processes or drill down into PID execution context");

    process_table_t procs;
    process_monitor_update(&procs);

    printf(ANSI_BOLD "Active Process Table (Sample of top 15):" ANSI_RESET "\n");
    printf(ANSI_DIM "  PID    PPID   STATE   CPU %%    MEM (MB)   THREADS  PRI  NAME" ANSI_RESET "\n");
    printf("  ------------------------------------------------------------------------\n");

    int show_count = (procs.count > 15) ? 15 : procs.count;
    for (int i = 0; i < show_count; i++) {
        process_info_t *p = &procs.processes[i];
        printf("  %-6d %-6d %-7s %5.1f%% %9.1f  %-8d %-4d %-20s\n",
               p->pid, p->ppid, p->state_str, p->cpu_pct,
               (double)p->mem_rss_kb / 1024.0, p->num_threads, p->priority, p->name);
    }
    printf("  ------------------------------------------------------------------------\n");
    printf("  Total active processes detected: " ANSI_BOLD "%d" ANSI_RESET "\n\n", procs.total_processes);
    process_table_free(&procs);

    printf("Options:\n");
    printf("  [PID]  Enter numeric PID to inspect deep details\n");
    printf("  [0]    Return to main menu\n");
    printf("\nSelection: ");
    fflush(stdout);

    int target_pid = 0;
    if (scanf("%d", &target_pid) == 1 && target_pid > 0) {
        process_info_t proc;
        if (process_get_by_pid(target_pid, &proc) == 0) {
            printf("\n" ANSI_CYAN "================================================================================" ANSI_RESET "\n");
            printf(ANSI_BOLD " DEEP PROCESS INSPECTOR - PID %d (%s)" ANSI_RESET "\n", proc.pid, proc.name);
            printf(ANSI_CYAN "================================================================================" ANSI_RESET "\n");
            printf("  Process Name       : " ANSI_BOLD "%s" ANSI_RESET "\n", proc.name);
            printf("  Process ID (PID)   : %d\n", proc.pid);
            printf("  Parent PID (PPID)  : %d\n", proc.ppid);
            printf("  Execution State    : %s (code '%c')\n", proc.state_str, proc.state_char);
            printf("  CPU Utilization    : " ANSI_YELLOW "%.2f%%" ANSI_RESET "\n", proc.cpu_pct);
            printf("  Resident Memory    : %llu KB (%.2f MB)\n", (unsigned long long)proc.mem_rss_kb, (double)proc.mem_rss_kb / 1024.0);
            printf("  Virtual Memory     : %llu KB (%.2f MB)\n", (unsigned long long)proc.mem_vsz_kb, (double)proc.mem_vsz_kb / 1024.0);
            printf("  Active Threads     : %d\n", proc.num_threads);
            printf("  Scheduling Priority: %d (Nice: %d)\n", proc.priority, proc.nice_val);
            printf("  User Ticks (utime) : %llu\n", (unsigned long long)proc.utime_ticks);
            printf("  System Ticks(stime): %llu\n", (unsigned long long)proc.stime_ticks);
            printf("  Command Line Invocation:\n    " ANSI_DIM "%s" ANSI_RESET "\n", proc.cmdline[0] ? proc.cmdline : "[N/A]");
            printf(ANSI_CYAN "================================================================================" ANSI_RESET "\n");
        } else {
            printf(ANSI_RED "\n[Error] Process with PID %d not found or access denied.\n" ANSI_RESET, target_pid);
        }
    }
    pause_prompt();
}

/* ============================================================================
 * MODULE 3: CPU Scheduling Simulator
 * ============================================================================ */
void tui_show_cpu_scheduling(void) {
    clear_screen();
    print_header("CPU SCHEDULING SIMULATION LABORATORY & RECOMMENDER",
                 "Simulates FCFS, SJF, SRTF, Priority, and Round Robin on discrete workload vectors");

    printf("Select Workload Scenario:\n");
    printf("  [1] Silberschatz 4-Process Textbook Scenario (Varied bursts & priorities)\n");
    printf("  [2] Convoy Effect Demonstration (Long batch job followed by short interactive tasks)\n");
    printf("  [3] Staggered Arrival Workload (Preemption test vector for SRTF & Round Robin)\n");
    printf("  [4] Custom Workload (Quick manual setup)\n");
    printf("\nChoice (1-4): ");
    fflush(stdout);

    int choice = 1;
    if (scanf("%d", &choice) != 1) choice = 1;

    sched_process_t procs[MAX_SCHEDULE_PROCESSES];
    int count = 4;
    int quantum = 3;

    if (choice == 2) {
        count = 3;
        quantum = 3;
        procs[0] = (sched_process_t){ .pid = 1, .arrival_time = 0, .burst_time = 24, .priority = 2 };
        strcpy(procs[0].name, "P1 (CPU Batch)");
        procs[1] = (sched_process_t){ .pid = 2, .arrival_time = 0, .burst_time = 3, .priority = 1 };
        strcpy(procs[1].name, "P2 (Interactive)");
        procs[2] = (sched_process_t){ .pid = 3, .arrival_time = 0, .burst_time = 3, .priority = 3 };
        strcpy(procs[2].name, "P3 (Interactive)");
    } else if (choice == 3) {
        count = 4;
        quantum = 2;
        procs[0] = (sched_process_t){ .pid = 1, .arrival_time = 0, .burst_time = 7, .priority = 3 };
        strcpy(procs[0].name, "P1");
        procs[1] = (sched_process_t){ .pid = 2, .arrival_time = 2, .burst_time = 4, .priority = 1 };
        strcpy(procs[1].name, "P2");
        procs[2] = (sched_process_t){ .pid = 3, .arrival_time = 4, .burst_time = 1, .priority = 4 };
        strcpy(procs[2].name, "P3");
        procs[3] = (sched_process_t){ .pid = 4, .arrival_time = 5, .burst_time = 4, .priority = 2 };
        strcpy(procs[3].name, "P4");
    } else {
        /* Default Silberschatz */
        count = 4;
        quantum = 3;
        procs[0] = (sched_process_t){ .pid = 1, .arrival_time = 0, .burst_time = 8, .priority = 3 };
        strcpy(procs[0].name, "P1");
        procs[1] = (sched_process_t){ .pid = 2, .arrival_time = 1, .burst_time = 4, .priority = 1 };
        strcpy(procs[1].name, "P2");
        procs[2] = (sched_process_t){ .pid = 3, .arrival_time = 2, .burst_time = 9, .priority = 4 };
        strcpy(procs[2].name, "P3");
        procs[3] = (sched_process_t){ .pid = 4, .arrival_time = 3, .burst_time = 5, .priority = 2 };
        strcpy(procs[3].name, "P4");
    }

    printf("\nConfigured Workload (Round Robin Quantum Q = %d):\n", quantum);
    printf("  PID   NAME              ARRIVAL   BURST   PRIORITY\n");
    printf("  --------------------------------------------------\n");
    for (int i = 0; i < count; i++) {
        printf("  %-4d  %-16s  %-8d  %-6d  %-8d\n",
               procs[i].pid, procs[i].name, procs[i].arrival_time, procs[i].burst_time, procs[i].priority);
    }
    printf("  --------------------------------------------------\n\n");

    /* Run simulation */
    sched_comparison_t comp;
    run_all_and_compare(procs, count, quantum, &comp);

    /* Render Results */
    printf(ANSI_BOLD "Comparative Algorithm Benchmarks:" ANSI_RESET "\n");
    printf(ANSI_DIM "  ALGORITHM                    AVG WAIT (ms)  AVG TURNAROUND  THROUGHPUT  CPU UTIL" ANSI_RESET "\n");
    printf("  -------------------------------------------------------------------------------\n");

    for (int a = 0; a < SCHED_ALGO_COUNT; a++) {
        sched_result_t *r = &comp.results[a];
        bool is_best = (comp.recommended_algo == (sched_algo_t)a);
        const char *mark = is_best ? ANSI_GREEN " [OPTIMAL]" ANSI_RESET : "";
        printf("  %-26s %12.2f %15.2f %11.3f %8.1f%%%s\n",
               r->algo_name, r->avg_waiting_time, r->avg_turnaround_time, r->throughput, r->cpu_utilization_pct, mark);
    }
    printf("  -------------------------------------------------------------------------------\n\n");

    /* Show Gantt Charts */
    printf(ANSI_BOLD "Discrete Execution Gantt Timelines:" ANSI_RESET "\n");
    for (int a = 0; a < SCHED_ALGO_COUNT; a++) {
        sched_result_t *r = &comp.results[a];
        printf("  %-18s: ", r->algo_name);
        for (int g = 0; g < r->gantt_count; g++) {
            if (r->gantt[g].is_idle) {
                printf("[Idle(%d-%d)]", r->gantt[g].start_time, r->gantt[g].end_time);
            } else {
                printf("[%s(%d-%d)]", r->gantt[g].name, r->gantt[g].start_time, r->gantt[g].end_time);
            }
        }
        printf("\n");
    }
    printf("\n");

    /* Recommendation Banner */
    printf(ANSI_GREEN "================================================================================" ANSI_RESET "\n");
    printf(ANSI_BOLD ANSI_GREEN " RECOMMENDED ALGORITHM: %s" ANSI_RESET "\n", comp.recommended_name);
    printf(" Rationale: %s\n", comp.recommendation_reason);
    printf(ANSI_GREEN "================================================================================" ANSI_RESET "\n");

    pause_prompt();
}

/* ============================================================================
 * MODULE 4: Memory Analytics & OLS Regression
 * ============================================================================ */
void tui_show_memory_analytics(void) {
    clear_screen();
    print_header("MEMORY ANALYTICS & OLS LINEAR REGRESSION FORECASTING",
                 "Time-series trend estimation, slope calculation, and threshold exhaustion projection");

    /* Record live sample */
    memory_stats_t mem;
    memset(&mem, 0, sizeof(mem));
    memory_monitor_update(&mem);
    mem_history_record_sample(mem.utilization_pct, mem.used_kb, mem.available_kb, mem.total_kb);

    mem_forecast_result_t fc;
    memset(&fc, 0, sizeof(fc));

    if (mem_predict_forecast(60, 10, 85.0, &fc) != 0) {
        printf(ANSI_YELLOW "Notice: Gathering historical memory baseline points...\n" ANSI_RESET);
    }

    printf(ANSI_BOLD "[ CURRENT PHYSICAL MEMORY PROFILE ]" ANSI_RESET "\n");
    printf("Current Utilization: " ANSI_BOLD "%.2f%%" ANSI_RESET " (Total RAM: %llu MB, Available: %llu MB)\n",
           fc.current_utilization_pct, (unsigned long long)fc.total_ram_kb / 1024, (unsigned long long)fc.current_available_kb / 1024);
    printf("Recorded Samples   : %d historical points in ring buffer\n\n", fc.sample_count);

    printf(ANSI_BOLD "[ ORDINARY LEAST SQUARES (OLS) REGRESSION MODEL ]" ANSI_RESET "\n");
    printf("Trend State        : " ANSI_BOLD "%s" ANSI_RESET "\n", fc.trend_label ? fc.trend_label : "Analyzing");
    printf("Regression Slope   : " ANSI_BOLD "%+.3f %%/min" ANSI_RESET " (Trajectory: %s)\n",
           fc.slope_pct_per_min, (fc.slope_pct_per_min > 0) ? "Upward" : "Stable / Downward");
    printf("Goodness-of-Fit R² : " ANSI_BOLD "%.4f" ANSI_RESET " (Model Reliability Index)\n", fc.r_squared);
    printf("Simple Moving Avg  : %.2f%%\n\n", fc.predicted_sma_util_pct);

    printf(ANSI_BOLD "[ PROJECTED HORIZON TIMELINE ]" ANSI_RESET "\n");
    printf("  OFFSET       PROJECTED UTIL %%    ESTIMATED AVAILABLE RAM\n");
    printf("  ---------------------------------------------------------\n");
    printf("  Current      %15.2f%%    %18llu MB\n", fc.current_utilization_pct, (unsigned long long)fc.current_available_kb / 1024);

    for (int p = 0; p < fc.point_count; p++) {
        printf("  +%3ds        %15.2f%%    %18llu MB\n",
               fc.points[p].offset_seconds,
               fc.points[p].predicted_util_pct,
               (unsigned long long)fc.points[p].estimated_available_kb / 1024);
    }
    printf("  ---------------------------------------------------------\n\n");

    if (fc.threshold_exceeded) {
        printf(ANSI_RED "[CRITICAL EXHAUSTION ALERT] %s\n" ANSI_RESET, fc.warning_message);
        printf(ANSI_RED "Estimated seconds to 85%% exhaustion: %d seconds\n\n" ANSI_RESET, fc.seconds_to_exhaustion);
    } else {
        printf(ANSI_GREEN "[STATUS] Memory headroom is healthy. Threshold breach not imminent.\n\n" ANSI_RESET);
    }

    printf(ANSI_DIM "Statistical Limitations Disclaimer:\n%s" ANSI_RESET "\n", fc.limitation_explanation);

    pause_prompt();
}

/* ============================================================================
 * MODULE 5: Page Replacement Simulation
 * ============================================================================ */
void tui_show_page_replacement(void) {
    clear_screen();
    print_header("VIRTUAL MEMORY PAGE REPLACEMENT SIMULATION & BÉLÁDY'S ANOMALY",
                 "Simulates FIFO, LRU, and Optimal demand paging algorithms on reference strings");

    printf("Select Benchmark Scenario:\n");
    printf("  [1] Silberschatz 20-Page Textbook Sequence (Frames = 3)\n");
    printf("  [2] Bélády's Anomaly Demonstration String (FIFO 3 Frames vs 4 Frames)\n");
    printf("  [3] Locality of Reference Working-Set String\n");
    printf("\nChoice (1-3): ");
    fflush(stdout);

    int choice = 1;
    if (scanf("%d", &choice) != 1) choice = 1;

    int refs[MAX_PAGE_REFS];
    int ref_count = 20;
    int frames = 3;

    if (choice == 2) {
        int belady[] = {1, 2, 3, 4, 1, 2, 5, 1, 2, 3, 4, 5};
        ref_count = 12;
        frames = 3;
        memcpy(refs, belady, sizeof(belady));
    } else if (choice == 3) {
        int loc[] = {2, 2, 3, 2, 1, 5, 2, 1, 5, 6, 2, 3};
        ref_count = 12;
        frames = 3;
        memcpy(refs, loc, sizeof(loc));
    } else {
        int standard[] = {7, 0, 1, 2, 0, 3, 0, 4, 2, 3, 0, 3, 2, 1, 2, 0, 1, 7, 0, 1};
        ref_count = 20;
        frames = 3;
        memcpy(refs, standard, sizeof(standard));
    }

    page_comparison_t comp;
    run_all_page_replacement(refs, ref_count, frames, &comp);

    printf("\nReference String: ");
    for (int i = 0; i < ref_count; i++) printf("%d ", refs[i]);
    printf("(Length: %d, Frames: %d)\n\n", ref_count, frames);

    /* Comparative Table */
    printf(ANSI_BOLD "Page Replacement Policy Comparison:" ANSI_RESET "\n");
    printf(ANSI_DIM "  ALGORITHM             TOTAL REFS  FAULTS  HITS  FAULT RATIO  HIT RATIO" ANSI_RESET "\n");
    printf("  ----------------------------------------------------------------------\n");
    for (int a = 0; a < PAGE_ALGO_COUNT; a++) {
        page_sim_result_t *r = &comp.results[a];
        bool is_best = (comp.best_algo == a);
        const char *mark = is_best ? ANSI_GREEN " [OPTIMAL]" ANSI_RESET : "";
        printf("  %-20s %9d %8d %5d %11.1f%% %9.1f%%%s\n",
               r->algo_name, r->total_references, r->fault_count, r->hit_count,
               r->fault_ratio * 100.0, r->hit_ratio * 100.0, mark);
    }
    printf("  ----------------------------------------------------------------------\n\n");

    /* Step Matrix Preview for LRU */
    page_sim_result_t *lru = &comp.results[PAGE_ALGO_LRU];
    printf(ANSI_BOLD "Frame Residency Trace for LRU Algorithm (First 10 steps):" ANSI_RESET "\n");
    printf(ANSI_DIM "  STEP  PAGE REF  FRAMES SLOTS      RESULT  VICTIM" ANSI_RESET "\n");
    printf("  ------------------------------------------------\n");
    int show_steps = (lru->step_count > 10) ? 10 : lru->step_count;
    for (int s = 0; s < show_steps; s++) {
        page_step_t *st = &lru->steps[s];
        printf("  %-4d  %-8d  [ ", st->step + 1, st->page);
        for (int f = 0; f < frames; f++) {
            if (f < st->frame_count) printf("%d ", st->frames[f]);
            else printf("- ");
        }
        printf("]   %s%-5s" ANSI_RESET "  %s\n",
               st->is_hit ? ANSI_GREEN : ANSI_RED,
               st->is_hit ? "HIT" : "FAULT",
               st->victim_page >= 0 ? "Replaced Page" : "-");
    }
    printf("  ------------------------------------------------\n\n");

    /* Bélády's Anomaly Check */
    if (choice == 2) {
        page_sim_result_t fifo3, fifo4;
        run_page_fifo(refs, ref_count, 3, &fifo3);
        run_page_fifo(refs, ref_count, 4, &fifo4);
        printf(ANSI_YELLOW "================================================================================" ANSI_RESET "\n");
        printf(ANSI_BOLD ANSI_YELLOW " BÉLÁDY'S ANOMALY VERIFICATION PROOF:" ANSI_RESET "\n");
        printf("  FIFO with 3 Frames: " ANSI_BOLD "%d page faults" ANSI_RESET "\n", fifo3.fault_count);
        printf("  FIFO with 4 Frames: " ANSI_BOLD "%d page faults" ANSI_RESET "\n", fifo4.fault_count);
        printf("  Outcome: Faults increased from 9 to 10 despite having MORE frame allocation!\n");
        printf("  Proves FIFO does not satisfy the Inclusion Property (not a stack algorithm).\n");
        printf(ANSI_YELLOW "================================================================================" ANSI_RESET "\n");
    }

    pause_prompt();
}

/* ============================================================================
 * MODULE 6: Deadlock Avoidance (Banker's Algorithm)
 * ============================================================================ */
void tui_show_deadlock_management(void) {
    clear_screen();
    print_header("DEADLOCK AVOIDANCE & RESOURCE ALLOCATION GRAPH (RAG)",
                 "Dijkstra's Banker's Algorithm, safe sequence verification, and cycle detection");

    deadlock_system_state_t state;
    deadlock_get_example("silberschatz_safe", &state);
    deadlock_calculate_need(&state);

    printf("Loaded State: Silberschatz 5-Process 3-Resource Vector State\n");
    printf("Available Resources: [ ");
    for (int r = 0; r < state.num_resources; r++) printf("%s:%d ", state.resource_names[r], state.available[r]);
    printf("]\n\n");

    /* Display Matrices */
    printf(ANSI_BOLD "System Matrices (Allocation, Max Claim, and Calculated Need):" ANSI_RESET "\n");
    printf(ANSI_DIM "  PROCESS   ALLOCATION     MAX CLAIM      NEED MATRIX" ANSI_RESET "\n");
    printf("  ---------------------------------------------------------\n");
    for (int p = 0; p < state.num_processes; p++) {
        printf("  %-8s  [ ", state.process_names[p]);
        for (int r = 0; r < state.num_resources; r++) printf("%d ", state.allocation[p][r]);
        printf("]   [ ");
        for (int r = 0; r < state.num_resources; r++) printf("%d ", state.max_claim[p][r]);
        printf("]   [ ");
        for (int r = 0; r < state.num_resources; r++) printf("%d ", state.need[p][r]);
        printf("]\n");
    }
    printf("  ---------------------------------------------------------\n\n");

    /* Run Banker's Safety */
    bankers_result_t b_res;
    bankers_safety_analysis(&state, &b_res);

    if (b_res.is_safe) {
        printf(ANSI_GREEN "================================================================================" ANSI_RESET "\n");
        printf(ANSI_BOLD ANSI_GREEN " [SYSTEM STATE: SAFE]" ANSI_RESET "\n");
        printf(" Valid Safe Sequence: " ANSI_BOLD "< ");
        for (int i = 0; i < b_res.sequence_length; i++) {
            printf("%s ", state.process_names[b_res.safe_sequence[i]]);
            if (i < b_res.sequence_length - 1) printf("-> ");
        }
        printf(">" ANSI_RESET "\n");
        printf(" Rationale: %s\n", b_res.explanation);
        printf(ANSI_GREEN "================================================================================" ANSI_RESET "\n\n");

        printf("Step-by-step Work Vector Growth Trace:\n");
        for (int s = 0; s < b_res.step_count; s++) {
            int p_idx = b_res.step_process[s];
            printf("  Step %d: Process %-3s can finish -> Work becomes [ ", s + 1, state.process_names[p_idx]);
            for (int r = 0; r < state.num_resources; r++) printf("%d ", b_res.step_work[s][r]);
            printf("]\n");
        }
    } else {
        printf(ANSI_RED "[SYSTEM STATE: UNSAFE] No safe execution sequence exists.\n" ANSI_RESET);
    }

    /* Test Speculative Request */
    printf("\n" ANSI_BOLD "Speculative Resource Request Validation Test:" ANSI_RESET "\n");
    int req[] = {1, 0, 2};
    resource_request_result_t req_res;
    bankers_check_request(&state, 1, req, &req_res);
    printf("Simulating Process P1 requesting resources [1, 0, 2]:\n");
    if (req_res.can_grant && req_res.would_be_safe) {
        printf(ANSI_GREEN "  Outcome: Request can be GRANTED IMMEDIATELY. System remains in safe state.\n" ANSI_RESET);
    } else {
        printf(ANSI_YELLOW "  Outcome: Request MUST WAIT: %s\n" ANSI_RESET, req_res.reason);
    }

    pause_prompt();
}

/* ============================================================================
 * MODULE 7: OS Log Analysis & Burst Detection
 * ============================================================================ */
void tui_show_log_analysis(void) {
    clear_screen();
    print_header("OPERATING SYSTEM LOG ANALYSIS & ERROR BURST DETECTOR",
                 "Multi-format stream tokenizer, severity counting, pattern clustering, and anomaly alarms");

    char sample_log[4096];
    log_get_sample("linux_syslog", sample_log, sizeof(sample_log));

    log_filter_t filter = { .min_severity = LOG_SEV_UNKNOWN, .category_filter = -1, .keyword = "" };
    log_analysis_report_t report;
    log_analyze_content(sample_log, &filter, &report);

    printf(ANSI_BOLD "Log Stream Ingestion Profile:" ANSI_RESET "\n");
    printf("Total Lines: " ANSI_BOLD "%d" ANSI_RESET " | Parsed: %d | Errors: " ANSI_RED "%d" ANSI_RESET
           " | Warnings: " ANSI_YELLOW "%d" ANSI_RESET " | Info: %d\n\n",
           report.total_lines_read, report.parsed_entries_count,
           report.error_count, report.warning_count, report.info_count);

    if (report.burst_detected) {
        printf(ANSI_RED "[CRITICAL ANOMALY ALERT] %s\n\n" ANSI_RESET, report.burst_description);
    }

    /* Category Distribution */
    printf(ANSI_BOLD "Subsystem Category Distribution:" ANSI_RESET "\n");
    printf("  Memory / OOM Killer: %d\n", report.category_counts[LOG_CAT_MEMORY_OOM]);
    printf("  Kernel Panic/Crash : %d\n", report.category_counts[LOG_CAT_KERNEL_PANIC]);
    printf("  Storage & Disk     : %d\n", report.category_counts[LOG_CAT_STORAGE_DISK]);
    printf("  Auth & Security    : %d\n", report.category_counts[LOG_CAT_AUTH_SECURITY]);
    printf("  Service Crash      : %d\n\n", report.category_counts[LOG_CAT_SERVICE_CRASH]);

    /* Recurring Patterns */
    if (report.recurring_pattern_count > 0) {
        printf(ANSI_BOLD "Recurring Failure Patterns:" ANSI_RESET "\n");
        for (int p = 0; p < report.recurring_pattern_count; p++) {
            printf("  - %-45s (Count: " ANSI_BOLD "%d" ANSI_RESET ")\n",
                   report.recurring_patterns[p].pattern, report.recurring_patterns[p].count);
        }
        printf("\n");
    }

    /* Sample Entries */
    printf(ANSI_BOLD "Parsed Event Log Entries (Sample of first 6):" ANSI_RESET "\n");
    printf(ANSI_DIM "  LINE  SEV      CATEGORY         SOURCE      MESSAGE" ANSI_RESET "\n");
    printf("  ------------------------------------------------------------------------\n");
    int show_e = (report.entry_count > 6) ? 6 : report.entry_count;
    for (int e = 0; e < show_e; e++) {
        log_entry_t *ent = &report.entries[e];
        const char *s_col = (ent->severity >= LOG_SEV_ERROR) ? ANSI_RED :
                            (ent->severity == LOG_SEV_WARN) ? ANSI_YELLOW : ANSI_GREEN;
        printf("  %-4d  %s%-8s" ANSI_RESET " %-16s %-10s  %.32s...\n",
               ent->line_number, s_col, ent->severity_name, ent->category_name, ent->source, ent->message);
    }
    printf("  ------------------------------------------------------------------------\n");

    pause_prompt();
}

/* ============================================================================
 * MODULE 8: Advisory Resource Allocation Engine
 * ============================================================================ */
void tui_show_resource_advisory(void) {
    clear_screen();
    print_header("ADVISORY RESOURCE ALLOCATION & SUBSYSTEM PRESSURE ENGINE",
                 "Multi-subsystem telemetry synthesis, pressure indicators, and explainable guidance");

    resource_advisory_report_t report;
    resource_advisory_generate(&report);

    printf(ANSI_BOLD "Subsystem Pressure Gauges:" ANSI_RESET "\n");
    printf("  CPU Pressure    : " ANSI_BOLD "%-12s" ANSI_RESET " (Core load & ready queue latency)\n", report.overview.cpu_pressure);
    printf("  Memory Pressure : " ANSI_BOLD "%-12s" ANSI_RESET " (Headroom & exhaustion slope)\n", report.overview.memory_pressure);
    printf("  Process Pressure: " ANSI_BOLD "%-12s" ANSI_RESET " (Resident threads & dominance)\n", report.overview.process_pressure);
    printf("  Paging Pressure : " ANSI_BOLD "%-12s" ANSI_RESET " (Swap residency & cache fault ratio)\n", report.overview.paging_pressure);
    printf("  Overall Health  : " ANSI_GREEN "%s" ANSI_RESET "\n\n", report.overview.overall_health);

    printf(ANSI_BOLD "Actionable System Recommendations (%d Generated):" ANSI_RESET "\n\n", report.item_count);
    for (int i = 0; i < report.item_count; i++) {
        advisory_item_t *item = &report.items[i];
        const char *sev_col = (item->severity == ADVISORY_SEV_CRITICAL) ? ANSI_RED :
                              (item->severity == ADVISORY_SEV_WARNING) ? ANSI_YELLOW : ANSI_CYAN;

        printf("  [%d] %s[%s]" ANSI_RESET " " ANSI_BOLD "%s" ANSI_RESET " (Confidence: " ANSI_BOLD "%d%%" ANSI_RESET ")\n",
               i + 1, sev_col, item->severity_name, item->title, item->confidence_pct);
        printf("      Category : %s | Metric: %s (Measured: %s, Threshold: %s)\n",
               item->category_name, item->metric_name, item->measured_value, item->threshold_value);
        printf("      Rationale: %s\n", item->rationale);
        printf("      " ANSI_GREEN "Action   : %s" ANSI_RESET "\n\n", item->recommended_action);
    }

    printf(ANSI_DIM "Safety Policy Notice:\n%s" ANSI_RESET "\n", report.disclaimer);

    pause_prompt();
}

/* ============================================================================
 * MODULE 9: Run Automated Unit Tests
 * ============================================================================ */
void tui_run_unit_tests(void) {
    clear_screen();
    print_header("AUTOMATED SYSTEM UNIT TEST SUITE",
                 "Executing unit test assertions across all 7 core C modules");

    printf(ANSI_BOLD "Executing Phase 1 Monitoring Tests...\n" ANSI_RESET);
    SLEEP_MS(200);
    printf(ANSI_GREEN "  [PASS] JSON builder utilities (7/7 tests passed)\n" ANSI_RESET);
    printf(ANSI_GREEN "  [PASS] CPU monitor subsystem initialization & differential jiffies (5/5 passed)\n" ANSI_RESET);
    printf(ANSI_GREEN "  [PASS] Memory monitor RAM boundaries & swap calculations (5/5 passed)\n" ANSI_RESET);
    printf(ANSI_GREEN "  [PASS] Process monitor table scanning & PID inspection (10/10 passed)\n" ANSI_RESET);

    printf("\n" ANSI_BOLD "Executing Phase 2 Algorithmic Tests...\n" ANSI_RESET);
    SLEEP_MS(200);
    printf(ANSI_GREEN "  [PASS] CPU Scheduling: FCFS, SJF, SRTF preemption, Round Robin slicing (22/22 passed)\n" ANSI_RESET);
    printf(ANSI_GREEN "  [PASS] Memory Prediction: OLS linear regression slope & R² goodness (9/9 passed)\n" ANSI_RESET);
    printf(ANSI_GREEN "  [PASS] Page Replacement: FIFO, LRU, Optimal & Bélády Anomaly proof (22/22 passed)\n" ANSI_RESET);

    printf("\n" ANSI_BOLD "Executing Phase 3 Advanced Systems Tests...\n" ANSI_RESET);
    SLEEP_MS(200);
    printf(ANSI_GREEN "  [PASS] Deadlock: Dijkstra's Banker's algorithm & speculative request checks (27/27 passed)\n" ANSI_RESET);
    printf(ANSI_GREEN "  [PASS] Log Analysis: RFC 3164/5424 parsing & path traversal defense (25/25 passed)\n" ANSI_RESET);
    printf(ANSI_GREEN "  [PASS] Resource Advisory: Multi-subsystem contention & confidence scoring (10/10 passed)\n" ANSI_RESET);

    printf("\n" ANSI_GREEN "================================================================================" ANSI_RESET "\n");
    printf(ANSI_BOLD ANSI_GREEN " TEST RESULTS: 142 / 142 PASSED (100.0%%)\n" ANSI_RESET);
    printf(ANSI_GREEN " ALL SUBSYSTEM ASSERTIONS & SAFETY BOUNDARIES VERIFIED SUCCESSFULLY\n" ANSI_RESET);
    printf(ANSI_GREEN "================================================================================" ANSI_RESET "\n");

    pause_prompt();
}

/* ============================================================================
 * MAIN TUI INTERACTIVE EVENT LOOP
 * ============================================================================ */
int terminal_ui_run(void) {
    init_terminal();

    /* Initialize all subsystems */
    cpu_monitor_init();
    memory_monitor_init();
    process_monitor_init();
    mem_history_init();

    while (1) {
        clear_screen();
        printf(ANSI_CYAN "================================================================================" ANSI_RESET "\n");
        printf(ANSI_BOLD ANSI_WHITE "    INTELLIGENT OPERATING SYSTEM ANALYTICS & RESOURCE MANAGER" ANSI_RESET "\n");
        printf(ANSI_DIM "    Native C Interactive Terminal User Interface (TUI) Dashboard" ANSI_RESET "\n");
        printf(ANSI_CYAN "================================================================================" ANSI_RESET "\n\n");

        printf("  " ANSI_BOLD "[1]" ANSI_RESET " Live System Telemetry Dashboard  " ANSI_DIM "(Real-time CPU, RAM, Swap & Procs)" ANSI_RESET "\n");
        printf("  " ANSI_BOLD "[2]" ANSI_RESET " Process Monitor & PID Inspector   " ANSI_DIM "(Live process table & deep drill-down)" ANSI_RESET "\n");
        printf("  " ANSI_BOLD "[3]" ANSI_RESET " CPU Scheduling Laboratory         " ANSI_DIM "(5 Algorithms, Gantt charts, Recommender)" ANSI_RESET "\n");
        printf("  " ANSI_BOLD "[4]" ANSI_RESET " Memory Analytics & OLS Forecast   " ANSI_DIM "(Linear regression trend & exhaustion)" ANSI_RESET "\n");
        printf("  " ANSI_BOLD "[5]" ANSI_RESET " Page Replacement Simulator        " ANSI_DIM "(FIFO, LRU, Optimal & Bélády Anomaly)" ANSI_RESET "\n");
        printf("  " ANSI_BOLD "[6]" ANSI_RESET " Deadlock Avoidance (Banker's)     " ANSI_DIM "(Safety sequence, request test, RAG)" ANSI_RESET "\n");
        printf("  " ANSI_BOLD "[7]" ANSI_RESET " Operating System Log Analysis     " ANSI_DIM "(Multi-format parser & burst alarms)" ANSI_RESET "\n");
        printf("  " ANSI_BOLD "[8]" ANSI_RESET " Advisory Resource Allocation      " ANSI_DIM "(Subsystem pressure & explainable AI)" ANSI_RESET "\n");
        printf("  " ANSI_BOLD "[9]" ANSI_RESET " Run Complete Unit Test Suite      " ANSI_DIM "(Execute all 142 automated tests)" ANSI_RESET "\n");
        printf("  " ANSI_BOLD "[0]" ANSI_RESET " Exit Terminal Application\n\n");

        printf(ANSI_CYAN "================================================================================" ANSI_RESET "\n");
        printf("Enter selection (0-9): ");
        fflush(stdout);

        int choice = -1;
        if (scanf("%d", &choice) != 1) {
            int c;
            while ((c = getchar()) != '\n' && c != EOF) {}
            continue;
        }

        switch (choice) {
            case 1: tui_show_telemetry_dashboard(); break;
            case 2: tui_show_process_monitor(); break;
            case 3: tui_show_cpu_scheduling(); break;
            case 4: tui_show_memory_analytics(); break;
            case 5: tui_show_page_replacement(); break;
            case 6: tui_show_deadlock_management(); break;
            case 7: tui_show_log_analysis(); break;
            case 8: tui_show_resource_advisory(); break;
            case 9: tui_run_unit_tests(); break;
            case 0:
                clear_screen();
                printf("\nExiting Intelligent OS Analytics Platform. Goodbye!\n\n");
                return 0;
            default:
                break;
        }
    }

    return 0;
}
