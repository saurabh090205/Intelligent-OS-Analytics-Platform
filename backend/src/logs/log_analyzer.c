#include "log_analyzer.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

static void str_to_lower(char *dest, const char *src, size_t max_len) {
    size_t i = 0;
    while (src[i] && i < max_len - 1) {
        dest[i] = (char)tolower((unsigned char)src[i]);
        i++;
    }
    dest[i] = '\0';
}

const char *log_severity_to_str(log_severity_t sev) {
    switch (sev) {
        case LOG_SEV_DEBUG: return "DEBUG";
        case LOG_SEV_INFO: return "INFO";
        case LOG_SEV_NOTICE: return "NOTICE";
        case LOG_SEV_WARN: return "WARN";
        case LOG_SEV_ERROR: return "ERROR";
        case LOG_SEV_CRITICAL: return "CRITICAL";
        case LOG_SEV_ALERT: return "ALERT";
        case LOG_SEV_EMERGENCY: return "EMERGENCY";
        default: return "UNKNOWN";
    }
}

log_severity_t log_severity_from_str(const char *str) {
    if (!str) return LOG_SEV_UNKNOWN;
    char lower[32];
    str_to_lower(lower, str, sizeof(lower));

    if (strstr(lower, "emerg") || strstr(lower, "fatal")) return LOG_SEV_EMERGENCY;
    if (strstr(lower, "alert")) return LOG_SEV_ALERT;
    if (strstr(lower, "crit")) return LOG_SEV_CRITICAL;
    if (strstr(lower, "err")) return LOG_SEV_ERROR;
    if (strstr(lower, "warn")) return LOG_SEV_WARN;
    if (strstr(lower, "notice")) return LOG_SEV_NOTICE;
    if (strstr(lower, "info")) return LOG_SEV_INFO;
    if (strstr(lower, "debug")) return LOG_SEV_DEBUG;

    return LOG_SEV_UNKNOWN;
}

const char *log_category_to_str(log_category_t cat) {
    switch (cat) {
        case LOG_CAT_MEMORY_OOM: return "Memory / OOM";
        case LOG_CAT_KERNEL_PANIC: return "Kernel & Hardware";
        case LOG_CAT_STORAGE_DISK: return "Storage & Disk";
        case LOG_CAT_AUTH_SECURITY: return "Security & Auth";
        case LOG_CAT_NETWORK: return "Network & Sockets";
        case LOG_CAT_SERVICE_CRASH: return "Service & Process";
        default: return "General Operating System";
    }
}

static log_category_t detect_category(const char *msg) {
    char lower[MAX_LOG_LINE_LEN];
    str_to_lower(lower, msg, sizeof(lower));

    if (strstr(lower, "out of memory") || strstr(lower, "oom-killer") ||
        strstr(lower, "page allocation failure") || strstr(lower, "cannot allocate memory") ||
        strstr(lower, "swap space low") || strstr(lower, "high memory pressure")) {
        return LOG_CAT_MEMORY_OOM;
    }

    if (strstr(lower, "kernel panic") || strstr(lower, "segmentation fault") ||
        strstr(lower, "segfault") || strstr(lower, "general protection") ||
        strstr(lower, "kernel bug") || strstr(lower, "hardware error") ||
        strstr(lower, "mknod") || strstr(lower, "trap divide error")) {
        return LOG_CAT_KERNEL_PANIC;
    }

    if (strstr(lower, "disk full") || strstr(lower, "no space left") ||
        strstr(lower, "i/o error") || strstr(lower, "read-only file system") ||
        strstr(lower, "bad sector") || strstr(lower, "bad block") ||
        strstr(lower, "ext4-fs error") || strstr(lower, "corrupt") ||
        strstr(lower, "chkdsk") || strstr(lower, "ntfs") ||
        strstr(lower, "filesystem") || strstr(lower, "inode")) {
        return LOG_CAT_STORAGE_DISK;
    }

    if (strstr(lower, "failed password") || strstr(lower, "authentication failure") ||
        strstr(lower, "permission denied") || strstr(lower, "sudo:") ||
        strstr(lower, "unauthorized") || strstr(lower, "invalid user") ||
        strstr(lower, "brute force") || strstr(lower, "pam_unix")) {
        return LOG_CAT_AUTH_SECURITY;
    }

    if (strstr(lower, "connection refused") || strstr(lower, "network unreachable") ||
        strstr(lower, "connection timed out") || strstr(lower, "reset by peer") ||
        strstr(lower, "link is down") || strstr(lower, "packet loss") ||
        strstr(lower, "dns lookup failed")) {
        return LOG_CAT_NETWORK;
    }

    if (strstr(lower, "core dumped") || strstr(lower, "terminated with signal") ||
        strstr(lower, "failed to start") || strstr(lower, "status=") ||
        strstr(lower, "main process exited") || strstr(lower, "service crashed") ||
        strstr(lower, "process died")) {
        return LOG_CAT_SERVICE_CRASH;
    }

    return LOG_CAT_GENERAL;
}

static log_severity_t infer_severity_from_keywords(const char *msg) {
    char lower[MAX_LOG_LINE_LEN];
    str_to_lower(lower, msg, sizeof(lower));

    if (strstr(lower, "panic") || strstr(lower, "fatal") || strstr(lower, "emergency")) {
        return LOG_SEV_EMERGENCY;
    }
    if (strstr(lower, "oom-killer") || strstr(lower, "out of memory") || strstr(lower, "critical")) {
        return LOG_SEV_CRITICAL;
    }
    if (strstr(lower, "error") || strstr(lower, "failed") || strstr(lower, "failure") ||
        strstr(lower, "corrupt") || strstr(lower, "segfault") || strstr(lower, "denied")) {
        return LOG_SEV_ERROR;
    }
    if (strstr(lower, "warn") || strstr(lower, "warning") || strstr(lower, "timeout") ||
        strstr(lower, "retry") || strstr(lower, "high memory")) {
        return LOG_SEV_WARN;
    }
    if (strstr(lower, "debug") || strstr(lower, "trace")) {
        return LOG_SEV_DEBUG;
    }

    return LOG_SEV_INFO;
}

int log_parse_line(const char *line, int line_num, log_entry_t *entry) {
    if (!line || !entry) return -1;
    memset(entry, 0, sizeof(log_entry_t));

    entry->line_number = line_num;
    strncpy(entry->raw_line, line, sizeof(entry->raw_line) - 1);

    /* Strip trailing newline / carriage return */
    size_t len = strlen(entry->raw_line);
    while (len > 0 && (entry->raw_line[len - 1] == '\r' || entry->raw_line[len - 1] == '\n')) {
        entry->raw_line[len - 1] = '\0';
        len--;
    }

    if (len == 0) {
        return -1; /* Empty line */
    }

    const char *p = entry->raw_line;
    while (*p && isspace((unsigned char)*p)) p++;

    /* Check Format 1: ISO 8601 Timestamp [2026-10-25T14:30:00Z] or 2026-10-25 14:30:00 */
    if (isdigit((unsigned char)p[0]) && isdigit((unsigned char)p[1]) &&
        isdigit((unsigned char)p[2]) && isdigit((unsigned char)p[3]) &&
        (p[4] == '-' || p[4] == '/')) {

        entry->is_supported_format = true;
        /* Copy timestamp */
        const char *ts_end = p;
        while (*ts_end && *ts_end != ' ' && *ts_end != '\t' && *ts_end != ']') ts_end++;
        if (*ts_end == ' ' && isdigit((unsigned char)*(ts_end + 1))) {
            /* Handles "YYYY-MM-DD HH:MM:SS" */
            while (*ts_end && *ts_end != ' ' && *ts_end != '\t' && *ts_end != ']') ts_end++;
            while (*ts_end && *ts_end != ' ' && *ts_end != '\t' && *ts_end != ']') ts_end++;
        }

        size_t tslen = ts_end - p;
        if (tslen >= sizeof(entry->timestamp_str)) tslen = sizeof(entry->timestamp_str) - 1;
        memcpy(entry->timestamp_str, p, tslen);
        entry->timestamp_str[tslen] = '\0';

        p = ts_end;
        while (*p && (*p == ' ' || *p == '\t' || *p == ']' || *p == '[')) p++;

        /* Look for [LEVEL] or LEVEL */
        char level_token[32] = {0};
        if (*p == '[') {
            const char *close_b = strchr(p, ']');
            if (close_b) {
                size_t ltok = close_b - p - 1;
                if (ltok >= sizeof(level_token)) ltok = sizeof(level_token) - 1;
                memcpy(level_token, p + 1, ltok);
                level_token[ltok] = '\0';
                p = close_b + 1;
            }
        } else {
            sscanf(p, "%31s", level_token);
            p += strlen(level_token);
        }

        while (*p && (*p == ' ' || *p == '\t' || *p == ':')) p++;

        entry->severity = log_severity_from_str(level_token);
        if (entry->severity == LOG_SEV_UNKNOWN) {
            entry->severity = infer_severity_from_keywords(p);
        }

        /* Check for Source: name[pid]: */
        char src_token[64] = {0};
        const char *colon = strchr(p, ':');
        if (colon && (colon - p) < 40) {
            size_t slen = colon - p;
            memcpy(src_token, p, slen);
            src_token[slen] = '\0';
            p = colon + 1;
            while (*p && isspace((unsigned char)*p)) p++;
            strncpy(entry->source, src_token, sizeof(entry->source) - 1);
        } else {
            strncpy(entry->source, "system", sizeof(entry->source) - 1);
        }

        strncpy(entry->message, p, sizeof(entry->message) - 1);
    }
    /* Check Format 2: Standard BSD / Linux Syslog (e.g. "Oct 25 14:32:10 hostname daemon[1234]: message") */
    else if ((strncmp(p, "Jan", 3) == 0 || strncmp(p, "Feb", 3) == 0 ||
              strncmp(p, "Mar", 3) == 0 || strncmp(p, "Apr", 3) == 0 ||
              strncmp(p, "May", 3) == 0 || strncmp(p, "Jun", 3) == 0 ||
              strncmp(p, "Jul", 3) == 0 || strncmp(p, "Aug", 3) == 0 ||
              strncmp(p, "Sep", 3) == 0 || strncmp(p, "Oct", 3) == 0 ||
              strncmp(p, "Nov", 3) == 0 || strncmp(p, "Dec", 3) == 0) &&
             strlen(p) > 16) {

        entry->is_supported_format = true;
        /* Copy "Mmm DD HH:MM:SS" (15 chars) */
        strncpy(entry->timestamp_str, p, 15);
        entry->timestamp_str[15] = '\0';
        p += 15;
        while (*p && isspace((unsigned char)*p)) p++;

        /* Next token is hostname, skip it */
        while (*p && !isspace((unsigned char)*p)) p++;
        while (*p && isspace((unsigned char)*p)) p++;

        /* Next token is service / source, up to ':' */
        const char *colon = strchr(p, ':');
        if (colon) {
            size_t slen = colon - p;
            if (slen >= sizeof(entry->source)) slen = sizeof(entry->source) - 1;
            memcpy(entry->source, p, slen);
            entry->source[slen] = '\0';
            p = colon + 1;
            while (*p && isspace((unsigned char)*p)) p++;
        } else {
            strncpy(entry->source, "kernel", sizeof(entry->source) - 1);
        }

        /* Message */
        strncpy(entry->message, p, sizeof(entry->message) - 1);
        entry->severity = infer_severity_from_keywords(entry->message);
    }
    /* Check Format 3: Generic Bracketed Format [TIMESTAMP] [LEVEL] [SOURCE] message */
    else if (*p == '[') {
        entry->is_supported_format = true;
        const char *b_close = strchr(p, ']');
        if (b_close) {
            size_t tslen = b_close - p - 1;
            if (tslen >= sizeof(entry->timestamp_str)) tslen = sizeof(entry->timestamp_str) - 1;
            memcpy(entry->timestamp_str, p + 1, tslen);
            entry->timestamp_str[tslen] = '\0';
            p = b_close + 1;
            while (*p && (*p == ' ' || *p == '\t')) p++;

            if (*p == '[') {
                const char *lvl_close = strchr(p, ']');
                if (lvl_close) {
                    char lvl[32] = {0};
                    size_t llen = lvl_close - p - 1;
                    if (llen >= sizeof(lvl)) llen = sizeof(lvl) - 1;
                    memcpy(lvl, p + 1, llen);
                    lvl[llen] = '\0';
                    entry->severity = log_severity_from_str(lvl);
                    p = lvl_close + 1;
                }
            }
        }
        while (*p && (*p == ' ' || *p == '\t' || *p == ':')) p++;
        strncpy(entry->source, "daemon", sizeof(entry->source) - 1);
        strncpy(entry->message, p, sizeof(entry->message) - 1);
        if (entry->severity == LOG_SEV_UNKNOWN) {
            entry->severity = infer_severity_from_keywords(entry->message);
        }
    }
    /* Check Format 4: Windows Event Line (e.g. "Level: Error, Source: Disk, Date: 2026-10-25, Message: ...") */
    else if (strstr(p, "Level:") || strstr(p, "Date:") || strstr(p, "Event:")) {
        entry->is_supported_format = true;
        const char *lvl = strstr(p, "Level:");
        if (lvl) {
            char lbuf[32] = {0};
            sscanf(lvl + 6, "%31[^,]", lbuf);
            entry->severity = log_severity_from_str(lbuf);
        }
        const char *src = strstr(p, "Source:");
        if (src) {
            sscanf(src + 7, "%47[^,]", entry->source);
        } else {
            strncpy(entry->source, "WinEvent", sizeof(entry->source) - 1);
        }
        const char *dt = strstr(p, "Date:");
        if (dt) {
            sscanf(dt + 5, "%47[^,]", entry->timestamp_str);
        }
        const char *msg = strstr(p, "Message:");
        if (msg) {
            strncpy(entry->message, msg + 8, sizeof(entry->message) - 1);
        } else {
            strncpy(entry->message, p, sizeof(entry->message) - 1);
        }
        if (entry->severity == LOG_SEV_UNKNOWN) {
            entry->severity = infer_severity_from_keywords(entry->message);
        }
    }
    else {
        /* Unrecognized / Freeform line */
        entry->is_supported_format = false;
        strncpy(entry->timestamp_str, "N/A", sizeof(entry->timestamp_str) - 1);
        strncpy(entry->source, "generic", sizeof(entry->source) - 1);
        strncpy(entry->message, p, sizeof(entry->message) - 1);
        entry->severity = infer_severity_from_keywords(entry->message);
    }

    /* Assign Severity Name */
    strncpy(entry->severity_name, log_severity_to_str(entry->severity), sizeof(entry->severity_name) - 1);

    /* Assign Category */
    entry->category = detect_category(entry->message);
    strncpy(entry->category_name, log_category_to_str(entry->category), sizeof(entry->category_name) - 1);

    return 0;
}

int log_analyze_content(const char *content, const log_filter_t *filter, log_analysis_report_t *report) {
    if (!content || !report) return -1;
    memset(report, 0, sizeof(log_analysis_report_t));

    const char *curr = content;
    int line_num = 1;
    int error_streak = 0;
    int max_error_streak = 0;

    /* Helper trackers for recurring patterns */
    int pattern_oom = 0;
    int pattern_segfault = 0;
    int pattern_disk_full = 0;
    int pattern_auth_fail = 0;
    int pattern_conn_refused = 0;
    int pattern_core_dump = 0;

    while (*curr) {
        char line_buf[MAX_LOG_LINE_LEN];
        size_t i = 0;
        while (*curr && *curr != '\n' && i < sizeof(line_buf) - 1) {
            line_buf[i++] = *curr++;
        }
        line_buf[i] = '\0';
        if (*curr == '\n') curr++;

        report->total_lines_read++;

        log_entry_t entry;
        if (log_parse_line(line_buf, line_num++, &entry) == 0) {
            report->parsed_entries_count++;
            if (!entry.is_supported_format) {
                report->unrecognized_lines_count++;
            }

            /* Global counters */
            if (entry.severity >= LOG_SEV_ERROR) {
                report->error_count++;
                error_streak++;
                if (error_streak > max_error_streak) {
                    max_error_streak = error_streak;
                }
            } else if (entry.severity == LOG_SEV_WARN) {
                report->warning_count++;
                error_streak++;
                if (error_streak > max_error_streak) {
                    max_error_streak = error_streak;
                }
            } else if (entry.severity >= LOG_SEV_INFO) {
                report->info_count++;
                error_streak = 0;
            } else {
                report->debug_count++;
                error_streak = 0;
            }

            /* Category distribution */
            if (entry.category >= 0 && entry.category < 7) {
                report->category_counts[entry.category]++;
            }

            /* Recurring pattern tracking */
            char lower_msg[MAX_LOG_LINE_LEN];
            str_to_lower(lower_msg, entry.message, sizeof(lower_msg));
            if (strstr(lower_msg, "out of memory") || strstr(lower_msg, "oom")) pattern_oom++;
            if (strstr(lower_msg, "segfault") || strstr(lower_msg, "segmentation fault")) pattern_segfault++;
            if (strstr(lower_msg, "disk full") || strstr(lower_msg, "no space")) pattern_disk_full++;
            if (strstr(lower_msg, "failed password") || strstr(lower_msg, "auth failure")) pattern_auth_fail++;
            if (strstr(lower_msg, "connection refused") || strstr(lower_msg, "reset by peer")) pattern_conn_refused++;
            if (strstr(lower_msg, "core dumped") || strstr(lower_msg, "service crashed")) pattern_core_dump++;

            /* Filter evaluation */
            bool matches = true;
            if (filter) {
                if (filter->min_severity > LOG_SEV_UNKNOWN && entry.severity < filter->min_severity) {
                    matches = false;
                }
                if (filter->category_filter >= 0 && entry.category != (log_category_t)filter->category_filter) {
                    matches = false;
                }
                if (filter->keyword[0] != '\0') {
                    char lower_kw[64];
                    str_to_lower(lower_kw, filter->keyword, sizeof(lower_kw));
                    if (!strstr(lower_msg, lower_kw) &&
                        !strstr(entry.raw_line, filter->keyword) &&
                        !strstr(entry.source, filter->keyword)) {
                        matches = false;
                    }
                }
            }

            if (matches && report->entry_count < MAX_LOG_ENTRIES) {
                report->entries[report->entry_count++] = entry;
            }
        }
    }

    /* Error Burst Analysis */
    if (max_error_streak >= 4 || report->error_count >= 5) {
        report->burst_detected = true;
        snprintf(report->burst_description, sizeof(report->burst_description),
                 "High-frequency error burst detected (%d total errors/critical events, max streak: %d). "
                 "System stability is degraded; immediate triage recommended.",
                 report->error_count, max_error_streak);
    } else {
        report->burst_detected = false;
        snprintf(report->burst_description, sizeof(report->burst_description),
                 "Log stream frequency is nominal. No cascading error bursts detected.");
    }

    /* Populate Recurring Patterns */
    int p_idx = 0;
    if (pattern_oom > 0 && p_idx < 8) {
        snprintf(report->recurring_patterns[p_idx].pattern, 64, "Out-Of-Memory (OOM Killer)");
        report->recurring_patterns[p_idx].count = pattern_oom;
        report->recurring_patterns[p_idx].severity = LOG_SEV_CRITICAL;
        p_idx++;
    }
    if (pattern_auth_fail > 0 && p_idx < 8) {
        snprintf(report->recurring_patterns[p_idx].pattern, 64, "Authentication Failures");
        report->recurring_patterns[p_idx].count = pattern_auth_fail;
        report->recurring_patterns[p_idx].severity = LOG_SEV_WARN;
        p_idx++;
    }
    if (pattern_segfault > 0 && p_idx < 8) {
        snprintf(report->recurring_patterns[p_idx].pattern, 64, "Segmentation Faults (SIGSEGV)");
        report->recurring_patterns[p_idx].count = pattern_segfault;
        report->recurring_patterns[p_idx].severity = LOG_SEV_ERROR;
        p_idx++;
    }
    if (pattern_disk_full > 0 && p_idx < 8) {
        snprintf(report->recurring_patterns[p_idx].pattern, 64, "Disk Full / Filesystem Errors");
        report->recurring_patterns[p_idx].count = pattern_disk_full;
        report->recurring_patterns[p_idx].severity = LOG_SEV_ERROR;
        p_idx++;
    }
    if (pattern_conn_refused > 0 && p_idx < 8) {
        snprintf(report->recurring_patterns[p_idx].pattern, 64, "Network Connection Refusals");
        report->recurring_patterns[p_idx].count = pattern_conn_refused;
        report->recurring_patterns[p_idx].severity = LOG_SEV_WARN;
        p_idx++;
    }
    if (pattern_core_dump > 0 && p_idx < 8) {
        snprintf(report->recurring_patterns[p_idx].pattern, 64, "Process Core Dumps");
        report->recurring_patterns[p_idx].count = pattern_core_dump;
        report->recurring_patterns[p_idx].severity = LOG_SEV_ERROR;
        p_idx++;
    }
    report->recurring_pattern_count = p_idx;

    return 0;
}

int log_is_safe_file_path(const char *file_path, char *err_msg, size_t err_size) {
    if (!file_path || file_path[0] == '\0') {
        if (err_msg && err_size > 0) snprintf(err_msg, err_size, "File path is empty or null");
        return -1;
    }

    /* 1. Strictly forbid directory traversal sequences */
    if (strstr(file_path, "..")) {
        if (err_msg && err_size > 0) {
            snprintf(err_msg, err_size, "Access Denied: Directory traversal ('..') is prohibited");
        }
        return -1;
    }

    /* 2. Convert to lowercase for security inspection */
    char lower[512] = {0};
    size_t len = strlen(file_path);
    if (len >= sizeof(lower)) len = sizeof(lower) - 1;
    for (size_t i = 0; i < len; i++) {
        lower[i] = (char)tolower((unsigned char)file_path[i]);
    }
    lower[len] = '\0';

    /* 3. Sensitive OS directories and files check */
    if (strstr(lower, "system32") || strstr(lower, "syswow64") ||
        strstr(lower, "\\config\\sam") || strstr(lower, "/config/sam") ||
        strstr(lower, "\\config\\system") || strstr(lower, "/config/system") ||
        strstr(lower, "\\config\\security") || strstr(lower, "/config/security") ||
        strstr(lower, "\\config\\software") || strstr(lower, "/config/software") ||
        strstr(lower, "system volume information") ||
        strstr(lower, ".ssh") || strstr(lower, "id_rsa") ||
        strstr(lower, ".env") || strstr(lower, "ntuser.dat") ||
        strstr(lower, "/etc/shadow") || strstr(lower, "/etc/gshadow") ||
        strstr(lower, "/etc/sudoers") || strstr(lower, "/etc/security") ||
        strstr(lower, "/proc/kcore")) {
        if (err_msg && err_size > 0) {
            snprintf(err_msg, err_size, "Access Denied: Path references a sensitive operating system target");
        }
        return -1;
    }

    /* 4. Forbid dangerous executables, binaries, and scripts */
    if (strstr(lower, ".exe") || strstr(lower, ".dll") || strstr(lower, ".sys") ||
        strstr(lower, ".bin") || strstr(lower, ".dat") || strstr(lower, ".so") ||
        strstr(lower, ".msi") || strstr(lower, ".iso") || strstr(lower, ".cmd") ||
        strstr(lower, ".bat") || strstr(lower, ".ps1") || strstr(lower, ".sh")) {
        if (err_msg && err_size > 0) {
            snprintf(err_msg, err_size, "Access Denied: Only plain text log files may be analyzed");
        }
        return -1;
    }

    return 0; /* Path is validated */
}

int log_analyze_file(const char *file_path, const log_filter_t *filter, log_analysis_report_t *report) {
    if (!file_path || !report) return -1;

    char sec_err[128] = {0};
    if (log_is_safe_file_path(file_path, sec_err, sizeof(sec_err)) != 0) {
        return -4; /* Access denied / security violation */
    }

    FILE *fp = fopen(file_path, "r");
    if (!fp) {
        return -2; /* File open error */
    }

    /* Check file size before buffering (5MB limit) */
    fseek(fp, 0, SEEK_END);
    long file_size = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    if (file_size < 0 || file_size > (5 * 1024 * 1024)) {
        fclose(fp);
        return -5; /* File too large for interactive analysis */
    }

    /* Read up to 512KB into buffer */
    const size_t max_bytes = 524288;
    char *buf = (char *)malloc(max_bytes + 1);
    if (!buf) {
        fclose(fp);
        return -3;
    }

    size_t read_bytes = fread(buf, 1, max_bytes, fp);
    buf[read_bytes] = '\0';
    fclose(fp);

    int res = log_analyze_content(buf, filter, report);
    free(buf);
    return res;
}

int log_get_sample(const char *sample_id, char *out_buf, size_t buf_size) {
    if (!sample_id || !out_buf || buf_size < 100) return -1;

    if (strcmp(sample_id, "linux_syslog") == 0) {
        snprintf(out_buf, buf_size,
            "Oct 25 14:10:01 host-srv systemd[1]: Started Daily apt upgrade and clean activities.\n"
            "Oct 25 14:10:05 host-srv sshd[1420]: Failed password for invalid user admin from 192.168.1.105 port 42890 ssh2\n"
            "Oct 25 14:10:07 host-srv sshd[1420]: Failed password for invalid user admin from 192.168.1.105 port 42890 ssh2\n"
            "Oct 25 14:11:15 host-srv kernel: [ 4210.124] EXT4-fs (sda1): warning: mounting unchecked fs, running e2fsck is recommended\n"
            "Oct 25 14:12:44 host-srv kernel: [ 4299.882] Out of memory: Kill process 8912 (stress-ng-vm) score 842 or sacrifice child\n"
            "Oct 25 14:12:44 host-srv kernel: [ 4299.885] oom-killer: gfp_mask=0x100cca(GFP_HIGHUSER_MOVABLE), order=0, oom_score_adj=0\n"
            "Oct 25 14:12:45 host-srv kernel: [ 4300.012] Killed process 8912 (stress-ng-vm) total-vm:4289120kB, anon-rss:3910240kB, file-rss:0kB\n"
            "Oct 25 14:14:02 host-srv systemd[1]: nginx.service: Main process exited, code=dumped, status=11/SEGV\n"
            "Oct 25 14:14:02 host-srv systemd[1]: nginx.service: Failed with result 'core-dump'.\n"
            "Oct 25 14:15:20 host-srv kernel: [ 4455.109] traps: worker[9104] general protection fault ip:7f890124 sp:7ffc1290 error:0\n"
            "Oct 25 14:16:30 host-srv postfix/pickup[4910]: fatal: open /etc/postfix/main.cf: No space left on device\n"
            "Oct 25 14:17:00 host-srv CRON[9200]: (root) CMD (   cd / && run-parts --report /etc/cron.hourly)\n"
        );
        return 0;
    }

    if (strcmp(sample_id, "windows_event") == 0) {
        snprintf(out_buf, buf_size,
            "Level: Information, Date: 2026-10-25 14:05:00, Source: Service Control Manager, Message: The Windows Update service entered the running state.\n"
            "Level: Warning, Date: 2026-10-25 14:08:12, Source: Disk, Message: The device, \\Device\\Harddisk0\\DR0, has a bad block.\n"
            "Level: Error, Date: 2026-10-25 14:10:45, Source: Ntfs, Message: The file system structure on the volume C: is corrupt and unusable. Please run the chkdsk utility on volume C:.\n"
            "Level: Warning, Date: 2026-10-25 14:12:30, Source: Resource-Exhaustion-Detector, Message: Windows successfully diagnosed a low virtual memory condition. The following programs consumed the most virtual memory: chrome.exe (14520) consumed 4294967296 bytes.\n"
            "Level: Error, Date: 2026-10-25 14:15:10, Source: Application Error, Message: Faulting application name: sqlservr.exe, version: 15.0.2000.5, faulting module name: ntdll.dll, exception code: 0xc0000005 (Access Violation).\n"
            "Level: Critical, Date: 2026-10-25 14:16:00, Source: Kernel-Power, Message: The system has rebooted without cleanly shutting down first. This error could be caused if the system stopped responding, crashed, or lost power unexpectedly.\n"
        );
        return 0;
    }

    if (strcmp(sample_id, "kernel_crash") == 0) {
        snprintf(out_buf, buf_size,
            "[    0.000000] Linux version 6.8.0-40-generic (buildd@lcy02-amd64-080) (gcc-13) #40-Ubuntu SMP PREEMPT_DYNAMIC\n"
            "[   48.120931] kernel: [ALERT] CPU 3: Machine Check Exception: Bank 4: b200000000070f0f\n"
            "[   48.120934] kernel: [EMERGENCY] Kernel panic - not syncing: Fatal machine check on CPU 3\n"
            "[   48.120938] kernel: [CRITICAL] Shutting down cpus with NMI\n"
            "[   48.121000] kernel: [EMERGENCY] Kernel Offset: 0x22000000 from 0xffffffff81000000\n"
            "[   48.121010] kernel: [CRITICAL] ---[ end Kernel panic - not syncing: Fatal machine check ]---\n"
        );
        return 0;
    }

    return -1;
}
