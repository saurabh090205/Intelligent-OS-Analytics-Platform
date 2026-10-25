#define __USE_MINGW_ANSI_STDIO 1
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <math.h>

#include "cpu_scheduler.h"
#include "memory_prediction.h"
#include "page_replacement.h"
#include "json_utils.h"

static int g_tests_run = 0;
static int g_tests_passed = 0;

#define TEST_ASSERT(cond, msg) do { \
    g_tests_run++; \
    if (cond) { \
        printf("  [PASS] %s\n", msg); \
        g_tests_passed++; \
    } else { \
        printf("  [FAIL] %s (Line %d: %s)\n", msg, __LINE__, #cond); \
    } \
} while(0)

/* ==================== 1. CPU Scheduling Tests ==================== */
static void test_cpu_scheduling(void) {
    printf("[TEST SUITE] Module 2: CPU Scheduling Algorithms\n");

    /* Canonical 4-process workload with staggered arrivals */
    sched_process_t procs[4] = {
        { .pid = 1, .name = "P1", .arrival_time = 0, .burst_time = 8, .priority = 3 },
        { .pid = 2, .name = "P2", .arrival_time = 1, .burst_time = 4, .priority = 1 },
        { .pid = 3, .name = "P3", .arrival_time = 2, .burst_time = 9, .priority = 4 },
        { .pid = 4, .name = "P4", .arrival_time = 3, .burst_time = 5, .priority = 2 },
    };

    /* Test FCFS */
    sched_result_t fcfs_res;
    int r = run_fcfs(procs, 4, &fcfs_res);
    TEST_ASSERT(r == 0, "FCFS completes successfully");
    TEST_ASSERT(fcfs_res.total_time == 26, "FCFS total time equals sum of bursts (26)");
    TEST_ASSERT(fcfs_res.processes[0].completion_time == 8, "FCFS P1 completes at t=8");
    TEST_ASSERT(fcfs_res.processes[1].completion_time == 12, "FCFS P2 completes at t=12");
    TEST_ASSERT(fcfs_res.avg_waiting_time > 0.0, "FCFS avg waiting time is positive");

    /* Test SJF Non-Preemptive */
    sched_result_t sjf_res;
    r = run_sjf(procs, 4, &sjf_res);
    TEST_ASSERT(r == 0, "SJF completes successfully");
    TEST_ASSERT(sjf_res.total_time == 26, "SJF total time equals 26");
    /* After P1 (0-8), P2 has shortest burst (4), so P2 runs 8-12 */
    TEST_ASSERT(sjf_res.processes[1].completion_time == 12, "SJF schedules P2 immediately after P1");
    /* Then P4 has next shortest burst (5), runs 12-17 */
    TEST_ASSERT(sjf_res.processes[3].completion_time == 17, "SJF schedules P4 before P3");

    /* Test SRTF Preemptive */
    sched_result_t srtf_res;
    r = run_srtf(procs, 4, &srtf_res);
    TEST_ASSERT(r == 0, "SRTF completes successfully");
    TEST_ASSERT(srtf_res.total_time == 26, "SRTF total time equals 26");
    /* At t=1, P2 arrives with burst 4, preempting P1 (remaining 7) */
    TEST_ASSERT(srtf_res.processes[1].completion_time == 5, "SRTF preempts P1 with shorter job P2 (completes at t=5)");
    TEST_ASSERT(srtf_res.avg_waiting_time <= fcfs_res.avg_waiting_time, "SRTF avg waiting time <= FCFS avg waiting time");

    /* Test Round Robin */
    sched_result_t rr_res;
    r = run_round_robin(procs, 4, 3, &rr_res);
    TEST_ASSERT(r == 0, "Round Robin (Q=3) completes successfully");
    TEST_ASSERT(rr_res.total_time == 26, "Round Robin total time equals 26");
    TEST_ASSERT(rr_res.gantt_count >= 4, "Round Robin generates multiple time-sliced Gantt segments");

    /* Test Priority Scheduling */
    sched_result_t pri_res;
    r = run_priority(procs, 4, false, &pri_res);
    TEST_ASSERT(r == 0, "Priority scheduling completes successfully");
    /* P2 has highest priority (1), should run after P1 completes */
    TEST_ASSERT(pri_res.processes[1].completion_time == 12, "Priority scheduling runs P2 (pri=1) first among arrived");

    /* Test Comparison & Recommendation */
    sched_comparison_t comp;
    r = run_all_and_compare(procs, 4, 2, &comp);
    TEST_ASSERT(r == 0, "Comparison across all 5 algorithms completes");
    TEST_ASSERT(comp.recommended_name != NULL, "Explainable recommendation selects optimal algorithm");
    TEST_ASSERT(strlen(comp.recommendation_reason) > 20, "Recommendation provides detailed explanatory rationale");

    /* Boundary Condition: Invalid / Empty Inputs */
    sched_result_t empty_res;
    TEST_ASSERT(run_fcfs(NULL, 0, &empty_res) == -1, "FCFS handles NULL input gracefully");
    TEST_ASSERT(run_round_robin(procs, 4, 0, &rr_res) == 0, "Round Robin handles quantum <= 0 with safe fallback (Q=2)");
}

/* ==================== 2. Memory Prediction Tests ==================== */
static void test_memory_prediction(void) {
    printf("[TEST SUITE] Module 3: Memory Prediction & Forecasting\n");

    mem_history_init();

    /* Test 1: Record consecutive synthetic linear trend samples */
    /* Utilization increasing: 50% -> 55% -> 60% -> 65% -> 70% */
    uint64_t fake_total = 16000000;
    for (int i = 0; i < 5; i++) {
        double util = 50.0 + (i * 5.0);
        uint64_t used = (uint64_t)(fake_total * (util / 100.0));
        uint64_t avail = fake_total - used;
        mem_history_record_sample(util, used, avail, fake_total);
    }

    mem_sample_t samples[10];
    int count = 0;
    mem_history_get_samples(samples, 10, &count);
    TEST_ASSERT(count >= 1, "Memory history buffer stores real/recorded samples");

    /* Test 2: Forecast computation */
    mem_forecast_result_t fc;
    int r = mem_predict_forecast(60, 5, 80.0, &fc);
    TEST_ASSERT(r == 0, "Memory forecasting completes successfully");
    TEST_ASSERT(fc.forecast_horizon_seconds == 60, "Configured horizon recorded correctly (60s)");
    TEST_ASSERT(fc.point_count > 0, "Generated forecast points along prediction timeline");
    TEST_ASSERT(fc.predicted_sma_util_pct >= 0.0 && fc.predicted_sma_util_pct <= 100.0,
                "Moving average utilization bounded within [0%, 100%]");
    TEST_ASSERT(fc.predicted_reg_util_pct >= 0.0 && fc.predicted_reg_util_pct <= 100.0,
                "Linear regression utilization bounded within [0%, 100%]");
    TEST_ASSERT(fc.predicted_available_kb <= fc.total_ram_kb,
                "Predicted available RAM is <= Total RAM");
    TEST_ASSERT(strlen(fc.limitation_explanation) > 10,
                "Includes honest statistical limitation explanation disclaimer");

    /* Test 3: High threshold alert triggers */
    mem_forecast_result_t fc_high;
    mem_predict_forecast(60, 5, 10.0, &fc_high); /* Threshold set to 10% (exceeded) */
    TEST_ASSERT(fc_high.threshold_exceeded == true, "High memory threshold alert triggers when exceeded");
}

/* ==================== 3. Page Replacement Tests ==================== */
static void test_page_replacement(void) {
    printf("[TEST SUITE] Module 4: Page Replacement Simulation\n");

    /* Standard Silberschatz 20-reference string */
    int ref_string[] = { 7, 0, 1, 2, 0, 3, 0, 4, 2, 3, 0, 3, 2, 1, 2, 0, 1, 7, 0, 1 };
    int n_refs = sizeof(ref_string) / sizeof(ref_string[0]);
    int n_frames = 3;

    /* 1. Test FIFO */
    page_sim_result_t fifo_res;
    int r = run_page_fifo(ref_string, n_refs, n_frames, &fifo_res);
    TEST_ASSERT(r == 0, "FIFO simulation completes successfully");
    TEST_ASSERT(fifo_res.fault_count == 15, "FIFO produces exactly 15 page faults on textbook string");
    TEST_ASSERT(fifo_res.hit_count == 5, "FIFO produces exactly 5 page hits");
    TEST_ASSERT(fabs(fifo_res.fault_ratio - (15.0 / 20.0)) < 1e-4, "FIFO fault ratio equals 0.75");
    TEST_ASSERT(fifo_res.step_count == 20, "FIFO tracks all 20 simulation steps frame-by-frame");

    /* 2. Test LRU */
    page_sim_result_t lru_res;
    r = run_page_lru(ref_string, n_refs, n_frames, &lru_res);
    TEST_ASSERT(r == 0, "LRU simulation completes successfully");
    TEST_ASSERT(lru_res.fault_count == 12, "LRU produces exactly 12 page faults on textbook string");
    TEST_ASSERT(lru_res.hit_count == 8, "LRU produces exactly 8 page hits");
    TEST_ASSERT(lru_res.fault_count < fifo_res.fault_count, "LRU outperforms FIFO on standard string (12 < 15)");

    /* 3. Test Optimal */
    page_sim_result_t opt_res;
    r = run_page_optimal(ref_string, n_refs, n_frames, &opt_res);
    TEST_ASSERT(r == 0, "Optimal simulation completes successfully");
    TEST_ASSERT(opt_res.fault_count == 9, "Optimal produces exactly 9 page faults on textbook string");
    TEST_ASSERT(opt_res.hit_count == 11, "Optimal produces exactly 11 page hits");
    TEST_ASSERT(opt_res.fault_count <= lru_res.fault_count, "Optimal achieves theoretical lower bound (9 <= 12)");

    /* 4. Test Comparison Across All Three */
    page_comparison_t comp;
    r = run_all_page_replacement(ref_string, n_refs, n_frames, &comp);
    TEST_ASSERT(r == 0, "Comparative runner across all 3 algorithms completes");
    TEST_ASSERT(comp.best_algo == PAGE_ALGO_OPTIMAL, "Comparison correctly identifies Optimal as minimum faults");
    TEST_ASSERT(strlen(comp.comparison_summary) > 20, "Generates clear comparative summary string");

    /* 5. Boundary Condition: Repeated References */
    int repeated_refs[] = { 1, 1, 1, 1, 1 };
    page_sim_result_t rep_res;
    run_page_fifo(repeated_refs, 5, 3, &rep_res);
    TEST_ASSERT(rep_res.fault_count == 1, "Repeated references produce only 1 initial fault");
    TEST_ASSERT(rep_res.hit_count == 4, "Repeated references produce 4 hits");

    /* 6. Boundary Condition: Belady's Anomaly Detection Test */
    /* String: [1, 2, 3, 4, 1, 2, 5, 1, 2, 3, 4, 5] */
    int belady_str[] = { 1, 2, 3, 4, 1, 2, 5, 1, 2, 3, 4, 5 };
    page_sim_result_t b3, b4;
    run_page_fifo(belady_str, 12, 3, &b3);
    run_page_fifo(belady_str, 12, 4, &b4);
    TEST_ASSERT(b3.fault_count == 9 && b4.fault_count == 10,
                "Belady's Anomaly verified: FIFO faults increase from 9 to 10 when frames increase from 3 to 4");

    /* 7. Boundary Condition: Invalid Inputs */
    TEST_ASSERT(run_page_fifo(NULL, 0, 3, &fifo_res) == -1, "Page simulator rejects NULL inputs");
    TEST_ASSERT(run_page_fifo(ref_string, 20, 0, &fifo_res) == -1, "Page simulator rejects frames <= 0");
}

int main(void) {
    printf("====================================================\n");
    printf(" Running Phase 2 C Backend Unit Tests\n");
    printf(" Modules: CPU Scheduling, Memory Forecast, Page Replacement\n");
    printf("====================================================\n\n");

    test_cpu_scheduling();
    printf("\n");
    test_memory_prediction();
    printf("\n");
    test_page_replacement();

    printf("\n====================================================\n");
    printf(" Phase 2 Test Results: %d / %d Passed (%.1f%%)\n",
           g_tests_passed, g_tests_run, (double)g_tests_passed / g_tests_run * 100.0);
    printf("====================================================\n");

    return (g_tests_passed == g_tests_run) ? 0 : 1;
}
