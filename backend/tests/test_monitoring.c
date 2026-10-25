#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "cpu_monitor.h"
#include "memory_monitor.h"
#include "process_monitor.h"
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

static void test_json_builder(void) {
    printf("[TEST SUITE] JSON Builder Utilities\n");
    json_builder_t jb;
    json_init(&jb, 128);

    json_obj_start(&jb);
    json_add_string(&jb, "name", "OS Monitor");
    json_add_int(&jb, "pid", 1024);
    json_add_double(&jb, "cpu_pct", 12.345, 2);
    json_add_bool(&jb, "active", true);
    json_add_null(&jb, "error");

    json_key(&jb, "items");
    json_arr_start(&jb);
    json_arr_add_int(&jb, 1);
    json_arr_add_int(&jb, 2);
    json_arr_add_string(&jb, "three");
    json_arr_end(&jb);

    json_obj_end(&jb);

    const char *str = json_get_string(&jb);
    TEST_ASSERT(str != NULL, "JSON builder generates non-null string");
    TEST_ASSERT(strstr(str, "\"name\":\"OS Monitor\"") != NULL, "JSON contains string key/value");
    TEST_ASSERT(strstr(str, "\"pid\":1024") != NULL, "JSON contains integer key/value");
    TEST_ASSERT(strstr(str, "\"cpu_pct\":12.35") != NULL, "JSON contains rounded double");
    TEST_ASSERT(strstr(str, "\"active\":true") != NULL, "JSON contains boolean");
    TEST_ASSERT(strstr(str, "\"error\":null") != NULL, "JSON contains null");
    TEST_ASSERT(strstr(str, "\"items\":[1,2,\"three\"]") != NULL, "JSON contains nested array");

    json_free(&jb);
}

static void test_cpu_monitor(void) {
    printf("[TEST SUITE] CPU Monitoring Subsystem\n");
    int init_res = cpu_monitor_init();
    TEST_ASSERT(init_res == 0, "CPU monitor initializes successfully");

    cpu_stats_t stats;
    memset(&stats, 0, sizeof(stats));
    int update_res = cpu_monitor_update(&stats);
    TEST_ASSERT(update_res == 0, "CPU monitor updates metrics without error");
    TEST_ASSERT(stats.core_count > 0, "CPU core count detected > 0");
    TEST_ASSERT(stats.overall_utilization >= 0.0 && stats.overall_utilization <= 100.0,
                "CPU utilization is bounded within [0%, 100%]");
    TEST_ASSERT(stats.total_jiffies > 0, "CPU total jiffies counter is positive");
}

static void test_memory_monitor(void) {
    printf("[TEST SUITE] Memory Monitoring Subsystem\n");
    int init_res = memory_monitor_init();
    TEST_ASSERT(init_res == 0, "Memory monitor initializes successfully");

    memory_stats_t mem;
    memset(&mem, 0, sizeof(mem));
    int update_res = memory_monitor_update(&mem);
    TEST_ASSERT(update_res == 0, "Memory monitor updates metrics without error");
    TEST_ASSERT(mem.total_kb > 0, "Total RAM reported > 0 KB");
    TEST_ASSERT(mem.available_kb <= mem.total_kb, "Available RAM is <= Total RAM");
    TEST_ASSERT(mem.utilization_pct >= 0.0 && mem.utilization_pct <= 100.0,
                "Memory utilization is bounded within [0%, 100%]");
    TEST_ASSERT(mem.used_kb <= mem.total_kb, "Used memory <= Total memory");
}

static void test_process_monitor(void) {
    printf("[TEST SUITE] Process Monitoring Subsystem\n");
    int init_res = process_monitor_init();
    TEST_ASSERT(init_res == 0, "Process monitor initializes successfully");

    process_table_t table;
    int update_res = process_monitor_update(&table);
    TEST_ASSERT(update_res == 0, "Process table scans running processes without error");
    TEST_ASSERT(table.total_processes > 0, "Discovered active system processes (> 0)");
    TEST_ASSERT(table.count > 0, "Populated process table entries");
    TEST_ASSERT(table.system_uptime_seconds > 0, "System uptime is positive");

    /* Validate first few processes */
    int checked = 0;
    for (int i = 0; i < table.count && i < 10; i++) {
        process_info_t *p = &table.processes[i];
        if (p->pid > 0 && strlen(p->name) > 0) {
            checked++;
        }
    }
    TEST_ASSERT(checked > 0, "Sampled processes have valid PIDs and non-empty names");

    /* Test single PID lookup */
    if (table.count > 0) {
        int test_pid = -1;
        for (int i = 0; i < table.count; i++) {
            if (table.processes[i].pid > 0) {
                test_pid = table.processes[i].pid;
                break;
            }
        }
        if (test_pid > 0) {
            process_info_t detail;
            int get_res = process_get_by_pid(test_pid, &detail);
            TEST_ASSERT(get_res == 0, "Lookup of existing PID succeeds");
            TEST_ASSERT(detail.pid == test_pid, "Returned process detail PID matches requested PID");
        }
    }

    /* Test non-existent PID lookup (boundary condition) */
    process_info_t dummy;
    int invalid_res = process_get_by_pid(99999999, &dummy);
    TEST_ASSERT(invalid_res != 0, "Lookup of non-existent PID (99999999) fails gracefully");

    process_table_free(&table);
}

int main(void) {
    printf("====================================================\n");
    printf(" Running Phase 1 C Backend Unit Tests\n");
    printf("====================================================\n");

    test_json_builder();
    test_cpu_monitor();
    test_memory_monitor();
    test_process_monitor();

    printf("\n====================================================\n");
    printf(" Test Results: %d / %d Passed (%.1f%%)\n",
           g_tests_passed, g_tests_run, (double)g_tests_passed / g_tests_run * 100.0);
    printf("====================================================\n");

    return (g_tests_passed == g_tests_run) ? 0 : 1;
}
