#ifndef LOG_ANALYZER_H
#define LOG_ANALYZER_H

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

#define MAX_LOG_ENTRIES 256
#define MAX_LOG_LINE_LEN 512

typedef enum {
    LOG_SEV_UNKNOWN = 0,
    LOG_SEV_DEBUG = 1,
    LOG_SEV_INFO = 2,
    LOG_SEV_NOTICE = 3,
    LOG_SEV_WARN = 4,
    LOG_SEV_ERROR = 5,
    LOG_SEV_CRITICAL = 6,
    LOG_SEV_ALERT = 7,
    LOG_SEV_EMERGENCY = 8
} log_severity_t;

typedef enum {
    LOG_CAT_GENERAL = 0,
    LOG_CAT_MEMORY_OOM = 1,
    LOG_CAT_KERNEL_PANIC = 2,
    LOG_CAT_STORAGE_DISK = 3,
    LOG_CAT_AUTH_SECURITY = 4,
    LOG_CAT_NETWORK = 5,
    LOG_CAT_SERVICE_CRASH = 6
} log_category_t;

typedef struct {
    int line_number;
    char raw_line[MAX_LOG_LINE_LEN];
    char timestamp_str[48];
    int64_t timestamp_epoch;
    char source[48];
    log_severity_t severity;
    char severity_name[16];
    log_category_t category;
    char category_name[32];
    char message[384];
    bool is_supported_format;
} log_entry_t;

typedef struct {
    log_severity_t min_severity;
    char keyword[64];
    int category_filter; /* -1 for all */
} log_filter_t;

typedef struct {
    char pattern[64];
    int count;
    log_severity_t severity;
} recurring_pattern_t;

typedef struct {
    int total_lines_read;
    int parsed_entries_count;
    int unrecognized_lines_count;

    int error_count;
    int warning_count;
    int info_count;
    int debug_count;

    int category_counts[7]; /* indexed by log_category_t */

    bool burst_detected;
    char burst_description[256];

    recurring_pattern_t recurring_patterns[8];
    int recurring_pattern_count;

    log_entry_t entries[MAX_LOG_ENTRIES];
    int entry_count;
} log_analysis_report_t;

/* Functions */
const char *log_severity_to_str(log_severity_t sev);
log_severity_t log_severity_from_str(const char *str);
const char *log_category_to_str(log_category_t cat);

int log_parse_line(const char *line, int line_num, log_entry_t *entry);
int log_analyze_content(const char *content, const log_filter_t *filter, log_analysis_report_t *report);
int log_analyze_file(const char *file_path, const log_filter_t *filter, log_analysis_report_t *report);
int log_is_safe_file_path(const char *file_path, char *err_msg, size_t err_size);

int log_get_sample(const char *sample_id, char *out_buf, size_t buf_size);

#endif /* LOG_ANALYZER_H */
