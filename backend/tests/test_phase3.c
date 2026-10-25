#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "deadlock.h"
#include "log_analyzer.h"
#include "resource_advisory.h"

#define TEST_PASS(msg) printf("  [PASS] %s\n", msg)
#define TEST_FAIL(msg) do { printf("  [FAIL] %s (Line %d)\n", msg, __LINE__); exit(1); } while(0)
#define EXPECT_TRUE(cond, msg) do { if (cond) { TEST_PASS(msg); } else { TEST_FAIL(msg); } } while(0)
#define EXPECT_EQ(a, b, msg) do { if ((a) == (b)) { TEST_PASS(msg); } else { printf("Expected %lld, got %lld: ", (long long)(b), (long long)(a)); TEST_FAIL(msg); } } while(0)

static void test_module5_deadlock(void) {
    printf("\n[TEST SUITE] Module 5: Deadlock Detection & Banker's Algorithm\n");

    /* 1. Validation test */
    deadlock_system_state_t bad_state;
    memset(&bad_state, 0, sizeof(bad_state));
    bad_state.num_processes = 0;
    char err_buf[128];
    EXPECT_TRUE(deadlock_validate_state(&bad_state, err_buf, sizeof(err_buf)) != 0,
                "Deadlock validator rejects 0 processes");

    bad_state.num_processes = 2;
    bad_state.num_resources = 1;
    bad_state.available[0] = -5;
    EXPECT_TRUE(deadlock_validate_state(&bad_state, err_buf, sizeof(err_buf)) != 0,
                "Deadlock validator rejects negative available resource");

    bad_state.available[0] = 5;
    bad_state.allocation[0][0] = 8;
    bad_state.max_claim[0][0] = 4;
    EXPECT_TRUE(deadlock_validate_state(&bad_state, err_buf, sizeof(err_buf)) != 0,
                "Deadlock validator rejects allocation exceeding max claim");

    /* 2. Classic Silberschatz Safe State */
    deadlock_system_state_t safe_state;
    EXPECT_EQ(deadlock_get_example("silberschatz_safe", &safe_state), 0,
              "Loads Silberschatz textbook safe example");
    EXPECT_EQ(safe_state.num_processes, 5, "Has 5 processes");
    EXPECT_EQ(safe_state.num_resources, 3, "Has 3 resource types");
    EXPECT_EQ(safe_state.need[0][0], 7, "P0 need for R0 is 7 (7 - 0)");
    EXPECT_EQ(safe_state.need[1][0], 1, "P1 need for R0 is 1 (3 - 2)");

    bankers_result_t safe_res;
    EXPECT_EQ(bankers_safety_analysis(&safe_state, &safe_res), 0,
              "Banker's safety analysis completes on safe state");
    EXPECT_TRUE(safe_res.is_safe, "System is confirmed in SAFE state");
    EXPECT_EQ(safe_res.sequence_length, 5, "Safe sequence contains all 5 processes");
    EXPECT_TRUE(strstr(safe_res.explanation, "valid execution sequence exists"),
                "Generates clear explanation for safe state");

    /* 3. Resource Request Handling */
    resource_request_result_t req_res;
    /* P1 requests [1, 0, 2]: Available is [3, 3, 2], Need is [1, 2, 2] -> Safe! */
    int valid_req[3] = {1, 0, 2};
    EXPECT_EQ(bankers_check_request(&safe_state, 1, valid_req, &req_res), 0,
              "Banker's evaluates valid request for P1");
    EXPECT_TRUE(req_res.can_grant, "Valid request can be granted immediately");
    EXPECT_TRUE(req_res.would_be_safe, "Resulting state remains SAFE");

    /* Request exceeding remaining Need */
    int excessive_req[3] = {2, 0, 0}; /* P1 only needs 1 of R0 */
    bankers_check_request(&safe_state, 1, excessive_req, &req_res);
    EXPECT_TRUE(!req_res.can_grant, "Rejects request exceeding process declared Need");

    /* Request exceeding Available */
    int unavail_req[3] = {0, 4, 0}; /* Available B is 3 */
    bankers_check_request(&safe_state, 0, unavail_req, &req_res);
    EXPECT_TRUE(!req_res.can_grant, "Rejects request exceeding currently Available resources");
    EXPECT_TRUE(strstr(req_res.reason, "Wait"), "Instructs process to wait");

    /* 4. Unsafe State Analysis */
    deadlock_system_state_t unsafe_state;
    EXPECT_EQ(deadlock_get_example("unsafe_state", &unsafe_state), 0,
              "Loads unsafe state example");
    bankers_result_t unsafe_res;
    bankers_safety_analysis(&unsafe_state, &unsafe_res);
    EXPECT_TRUE(!unsafe_res.is_safe, "System is correctly identified as UNSAFE");
    EXPECT_TRUE(strstr(unsafe_res.explanation, "An unsafe state is NOT synonymous with deadlock"),
                "Explanation explicitly distinguishes unsafe state from deadlock");

    /* 5. Deadlock Detection */
    deadlock_system_state_t cycle_state;
    EXPECT_EQ(deadlock_get_example("deadlock_cycle", &cycle_state), 0,
              "Loads deadlock cycle example");
    deadlock_detection_result_t det_res;
    EXPECT_EQ(deadlock_detect(&cycle_state, NULL, &det_res), 0,
              "Deadlock detection algorithm completes");
    EXPECT_TRUE(det_res.is_deadlocked, "Detects active circular deadlock");
    EXPECT_TRUE(det_res.deadlocked_count >= 2, "Identifies deadlocked processes in cycle");

    /* Deadlock detection on safe state */
    deadlock_detection_result_t safe_det;
    deadlock_detect(&safe_state, NULL, &safe_det);
    EXPECT_TRUE(!safe_det.is_deadlocked, "Deadlock detection reports no deadlock on safe state");

    /* NULL pointers safety */
    EXPECT_TRUE(bankers_safety_analysis(NULL, NULL) == -1, "Handles NULL pointer safely");
}

static void test_module6_logs(void) {
    printf("\n[TEST SUITE] Module 6: Operating System Log Analysis\n");

    /* 1. Parse Syslog line */
    const char *syslog_line = "Oct 25 14:12:44 host kernel: [ 4299.882] Out of memory: Kill process 8912 (stress-ng-vm) score 842";
    log_entry_t entry1;
    EXPECT_EQ(log_parse_line(syslog_line, 1, &entry1), 0, "Parses standard Linux syslog line");
    EXPECT_TRUE(entry1.is_supported_format, "Recognized as supported format");
    EXPECT_EQ(entry1.category, LOG_CAT_MEMORY_OOM, "Categorized as Memory / OOM");
    EXPECT_TRUE(entry1.severity >= LOG_SEV_ERROR, "Detected as high severity (CRITICAL/ERROR)");
    EXPECT_TRUE(strcmp(entry1.source, "kernel") == 0, "Extracted source as 'kernel'");

    /* 2. Parse Windows Event line */
    const char *win_line = "Level: Error, Date: 2026-10-25 14:10:45, Source: Ntfs, Message: The file system structure on the volume C: is corrupt";
    log_entry_t entry2;
    EXPECT_EQ(log_parse_line(win_line, 2, &entry2), 0, "Parses Windows event format line");
    EXPECT_TRUE(entry2.is_supported_format, "Windows line recognized as supported format");
    EXPECT_EQ(entry2.category, LOG_CAT_STORAGE_DISK, "Categorized as Storage & Disk");
    EXPECT_EQ(entry2.severity, LOG_SEV_ERROR, "Parsed severity as ERROR");

    /* 3. Parse ISO 8601 format line */
    const char *iso_line = "2026-10-25T14:14:02Z [WARN] sshd: Failed password for invalid user admin from 10.0.0.1";
    log_entry_t entry3;
    EXPECT_EQ(log_parse_line(iso_line, 3, &entry3), 0, "Parses ISO 8601 formatted line");
    EXPECT_EQ(entry3.category, LOG_CAT_AUTH_SECURITY, "Categorized as Security & Auth");
    EXPECT_EQ(entry3.severity, LOG_SEV_WARN, "Parsed severity as WARN");

    /* 4. Parse Freeform / Unrecognized line */
    const char *freeform = "Custom arbitrary log statement without recognized header";
    log_entry_t entry4;
    EXPECT_EQ(log_parse_line(freeform, 4, &entry4), 0, "Parses freeform log without crashing");
    EXPECT_TRUE(!entry4.is_supported_format, "Flags unsupported / freeform format");

    /* 5. Full Log Stream Analysis & Burst Detection */
    char sample_buf[4096];
    EXPECT_EQ(log_get_sample("linux_syslog", sample_buf, sizeof(sample_buf)), 0,
              "Loads Linux syslog sample");

    log_filter_t filter;
    memset(&filter, 0, sizeof(filter));
    filter.category_filter = -1;

    static log_analysis_report_t report;
    EXPECT_EQ(log_analyze_content(sample_buf, &filter, &report), 0,
              "Analyzes full log stream");
    EXPECT_TRUE(report.total_lines_read > 5, "Processed multiple log lines");
    EXPECT_TRUE(report.error_count > 0, "Detected error entries in sample");
    EXPECT_TRUE(report.category_counts[LOG_CAT_MEMORY_OOM] > 0, "Recorded OOM category count");
    EXPECT_TRUE(report.category_counts[LOG_CAT_AUTH_SECURITY] > 0, "Recorded Auth category count");
    EXPECT_TRUE(report.recurring_pattern_count > 0, "Extracted recurring error patterns");

    /* 6. Filter by keyword */
    strncpy(filter.keyword, "sshd", sizeof(filter.keyword));
    static log_analysis_report_t ssh_report;
    log_analyze_content(sample_buf, &filter, &ssh_report);
    EXPECT_TRUE(ssh_report.entry_count > 0, "Keyword filter returned matching entries");
    for (int i = 0; i < ssh_report.entry_count; i++) {
        EXPECT_TRUE(strstr(ssh_report.entries[i].raw_line, "sshd") != NULL, "Filtered line contains 'sshd'");
    }

    /* 7. Security: File Path Validation & Traversal Defense */
    char sec_msg[128] = {0};
    EXPECT_TRUE(log_is_safe_file_path("../../Windows/System32/config/SAM", sec_msg, sizeof(sec_msg)) != 0,
                "Rejects path traversal ('..') attempt");
    EXPECT_TRUE(log_is_safe_file_path("C:\\Windows\\System32\\cmd.exe", sec_msg, sizeof(sec_msg)) != 0,
                "Rejects access to system32 executable");
    EXPECT_TRUE(log_is_safe_file_path("/etc/shadow", sec_msg, sizeof(sec_msg)) != 0,
                "Rejects access to /etc/shadow sensitive file");
    EXPECT_TRUE(log_is_safe_file_path("backend/tests/sample.log", sec_msg, sizeof(sec_msg)) == 0,
                "Accepts safe workspace log path");

    static log_analysis_report_t sec_report;
    EXPECT_EQ(log_analyze_file("../sensitive.log", &filter, &sec_report), -4,
              "log_analyze_file enforces access denial for traversal path");
}

static void test_module7_advisory(void) {
    printf("\n[TEST SUITE] Module 7: Advisory Resource Allocation\n");

    static resource_advisory_report_t adv_report;
    EXPECT_EQ(resource_advisory_generate(&adv_report), 0, "Generates resource advisory report");
    EXPECT_TRUE(adv_report.item_count >= 3, "Generated at least 3 distinct advisory recommendations");
    EXPECT_TRUE(strlen(adv_report.overview.overall_health) > 0, "Overall system health is assessed");
    EXPECT_TRUE(strlen(adv_report.overview.cpu_pressure) > 0, "CPU pressure is evaluated");
    EXPECT_TRUE(strlen(adv_report.overview.memory_pressure) > 0, "Memory pressure is evaluated");

    /* Verify each advisory item structure */
    for (int i = 0; i < adv_report.item_count; i++) {
        advisory_item_t *item = &adv_report.items[i];
        EXPECT_TRUE(strlen(item->title) > 0, "Advisory item has valid title");
        EXPECT_TRUE(strlen(item->rationale) > 0, "Advisory item has supporting rationale");
        EXPECT_TRUE(strlen(item->recommended_action) > 0, "Advisory item has recommended action");
        EXPECT_TRUE(item->confidence_pct >= 50 && item->confidence_pct <= 100, "Confidence is within [50%, 100%]");
    }

    /* Verify disclaimer adheres to safety boundary */
    EXPECT_TRUE(strstr(adv_report.disclaimer, "ADVISORY NOTICE") != NULL,
                "Report includes formal advisory notice");
    EXPECT_TRUE(strstr(adv_report.disclaimer, "does NOT automatically terminate processes") != NULL,
                "Disclaimer confirms platform does not kill processes automatically");
}

int main(void) {
    printf("====================================================\n");
    printf(" Running Phase 3 C Backend Unit Tests\n");
    printf(" Modules: Deadlock Analysis, OS Log Analyzer, Advisory Engine\n");
    printf("====================================================\n");

    test_module5_deadlock();
    test_module6_logs();
    test_module7_advisory();

    printf("\n====================================================\n");
    printf(" Phase 3 Test Results: All Unit Tests Passed (100.0%%)\n");
    printf("====================================================\n");

    return 0;
}
