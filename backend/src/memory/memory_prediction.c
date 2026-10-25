#define __USE_MINGW_ANSI_STDIO 1
#include "memory_prediction.h"
#include "memory_monitor.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>

static mem_sample_t s_history[MAX_MEM_HISTORY];
static int s_head = 0;
static int s_count = 0;

int mem_history_init(void) {
    s_head = 0;
    s_count = 0;
    memset(s_history, 0, sizeof(s_history));

    /* Populate with current real measurement */
    memory_stats_t mem;
    if (memory_monitor_update(&mem) == 0) {
        mem_history_record_sample(mem.utilization_pct, mem.used_kb, mem.available_kb, mem.total_kb);
    }
    return 0;
}

int mem_history_record_sample(double util_pct, uint64_t used_kb, uint64_t avail_kb, uint64_t total_kb) {
    uint64_t now_ts = (uint64_t)time(NULL);

    if (s_count > 0) {
        int last_idx = (s_head - 1 + MAX_MEM_HISTORY) % MAX_MEM_HISTORY;
        if (now_ts <= s_history[last_idx].timestamp) {
            now_ts = s_history[last_idx].timestamp + 1;
        }
    }

    s_history[s_head].timestamp = now_ts;
    s_history[s_head].utilization_pct = util_pct;
    s_history[s_head].used_kb = used_kb;
    s_history[s_head].available_kb = avail_kb;
    s_history[s_head].total_kb = total_kb;

    s_head = (s_head + 1) % MAX_MEM_HISTORY;
    if (s_count < MAX_MEM_HISTORY) {
        s_count++;
    }
    return 0;
}

int mem_history_get_samples(mem_sample_t *out_samples, int max_count, int *out_count) {
    if (!out_samples || max_count <= 0) return -1;
    int available = (s_count < max_count) ? s_count : max_count;

    int start_idx = (s_head - s_count + MAX_MEM_HISTORY) % MAX_MEM_HISTORY;
    for (int i = 0; i < available; i++) {
        int idx = (start_idx + i) % MAX_MEM_HISTORY;
        out_samples[i] = s_history[idx];
    }

    if (out_count) *out_count = available;
    return 0;
}

int mem_predict_forecast(int horizon_seconds, int window_size, double threshold_pct,
                         mem_forecast_result_t *out_forecast) {
    if (!out_forecast) return -1;
    memset(out_forecast, 0, sizeof(mem_forecast_result_t));

    if (horizon_seconds <= 0) horizon_seconds = 60;
    if (horizon_seconds > 300) horizon_seconds = 300;
    if (window_size <= 0) window_size = 10;
    if (threshold_pct <= 0.0 || threshold_pct > 100.0) threshold_pct = 85.0;

    out_forecast->forecast_horizon_seconds = horizon_seconds;
    out_forecast->threshold_pct = threshold_pct;
    out_forecast->seconds_to_exhaustion = -1;

    /* Ensure we have up-to-date sample */
    memory_stats_t cur_mem;
    if (memory_monitor_update(&cur_mem) == 0) {
        mem_history_record_sample(cur_mem.utilization_pct, cur_mem.used_kb,
                                  cur_mem.available_kb, cur_mem.total_kb);
        out_forecast->current_utilization_pct = cur_mem.utilization_pct;
        out_forecast->current_available_kb = cur_mem.available_kb;
        out_forecast->total_ram_kb = cur_mem.total_kb;
    } else {
        out_forecast->total_ram_kb = 16777216;
    }

    mem_sample_t samples[MAX_MEM_HISTORY];
    int n = 0;
    mem_history_get_samples(samples, MAX_MEM_HISTORY, &n);
    out_forecast->sample_count = n;

    if (n < 2) {
        /* Insufficient historical points: synthesize baseline forecast from current state */
        double cur_util = out_forecast->current_utilization_pct;
        out_forecast->trend = MEM_TREND_STABLE;
        out_forecast->trend_label = "Stable (Insufficient History)";
        out_forecast->slope_pct_per_min = 0.0;
        out_forecast->r_squared = 1.0;
        out_forecast->predicted_sma_util_pct = cur_util;
        out_forecast->predicted_reg_util_pct = cur_util;
        out_forecast->predicted_available_kb = out_forecast->current_available_kb;

        int num_pts = 6;
        out_forecast->point_count = num_pts;
        uint64_t base_ts = (uint64_t)time(NULL);
        int step = horizon_seconds / num_pts;

        for (int i = 0; i < num_pts; i++) {
            int off = (i + 1) * step;
            out_forecast->points[i].timestamp = base_ts + off;
            out_forecast->points[i].offset_seconds = off;
            out_forecast->points[i].predicted_util_pct = cur_util;
            out_forecast->points[i].estimated_used_kb = (uint64_t)(out_forecast->total_ram_kb * (cur_util / 100.0));
            out_forecast->points[i].estimated_available_kb = out_forecast->total_ram_kb - out_forecast->points[i].estimated_used_kb;
        }

        if (cur_util >= threshold_pct) {
            out_forecast->threshold_exceeded = true;
            snprintf(out_forecast->warning_message, sizeof(out_forecast->warning_message),
                "WARNING: Current memory utilization (%.1f%%) exceeds configured safety threshold (%.0f%%).",
                cur_util, threshold_pct);
        } else {
            out_forecast->threshold_exceeded = false;
            snprintf(out_forecast->warning_message, sizeof(out_forecast->warning_message),
                "Memory utilization (%.1f%%) is currently within safe operational threshold (%.0f%%).",
                cur_util, threshold_pct);
        }

        snprintf(out_forecast->limitation_explanation, sizeof(out_forecast->limitation_explanation),
            "Fewer than 2 historical measurements recorded. Gathering real-time /proc telemetry samples.");
        return 0;
    }

    /* 1. Moving Average (SMA) over window_size */
    int w = (n < window_size) ? n : window_size;
    double sma_sum = 0.0;
    for (int i = n - w; i < n; i++) {
        sma_sum += samples[i].utilization_pct;
    }
    out_forecast->predicted_sma_util_pct = sma_sum / w;

    /* 2. Linear Regression (Least Squares) */
    /* x_i = timestamp - t_0 (in seconds), y_i = utilization_pct */
    uint64_t t0 = samples[0].timestamp;
    double sum_x = 0.0, sum_y = 0.0, sum_xy = 0.0, sum_x2 = 0.0, sum_y2 = 0.0;

    for (int i = 0; i < n; i++) {
        double x = (double)(samples[i].timestamp - t0);
        double y = samples[i].utilization_pct;
        sum_x += x;
        sum_y += y;
        sum_xy += (x * y);
        sum_x2 += (x * x);
        sum_y2 += (y * y);
    }

    double denom = (n * sum_x2) - (sum_x * sum_x);
    double slope = 0.0;
    double intercept = sum_y / n;

    if (fabs(denom) > 1e-9) {
        slope = ((n * sum_xy) - (sum_x * sum_y)) / denom;
        intercept = (sum_y - (slope * sum_x)) / n;
    }

    /* Compute R-squared */
    double denom_r = ((n * sum_x2) - (sum_x * sum_x)) * ((n * sum_y2) - (sum_y * sum_y));
    if (denom_r > 1e-9) {
        double num_r = (n * sum_xy) - (sum_x * sum_y);
        out_forecast->r_squared = (num_r * num_r) / denom_r;
    } else {
        out_forecast->r_squared = 1.0;
    }

    out_forecast->slope_pct_per_min = slope * 60.0;

    /* Determine Trend */
    if (out_forecast->slope_pct_per_min > 0.25) {
        out_forecast->trend = MEM_TREND_INCREASING;
        out_forecast->trend_label = "Increasing (Memory Pressure Rising)";
    } else if (out_forecast->slope_pct_per_min < -0.25) {
        out_forecast->trend = MEM_TREND_DECREASING;
        out_forecast->trend_label = "Decreasing (Memory Being Freed)";
    } else {
        out_forecast->trend = MEM_TREND_STABLE;
        out_forecast->trend_label = "Stable (Equilibrium)";
    }

    /* Target future prediction at horizon_seconds */
    double last_x = (double)(samples[n - 1].timestamp - t0);
    double future_x = last_x + horizon_seconds;
    double reg_pred = (slope * future_x) + intercept;
    if (reg_pred < 0.0) reg_pred = 0.0;
    if (reg_pred > 100.0) reg_pred = 100.0;
    out_forecast->predicted_reg_util_pct = reg_pred;

    /* Available memory estimate */
    if (out_forecast->total_ram_kb > 0) {
        double avail_fraction = (100.0 - reg_pred) / 100.0;
        if (avail_fraction < 0.0) avail_fraction = 0.0;
        out_forecast->predicted_available_kb = (uint64_t)(out_forecast->total_ram_kb * avail_fraction);
    }

    /* Threshold check & time to exhaustion */
    if (reg_pred >= threshold_pct || out_forecast->current_utilization_pct >= threshold_pct) {
        out_forecast->threshold_exceeded = true;
        snprintf(out_forecast->warning_message, sizeof(out_forecast->warning_message),
            "WARNING: Memory utilization is projected to reach %.1f%% (exceeding %.0f%% safety threshold).",
            reg_pred, threshold_pct);
    } else {
        out_forecast->threshold_exceeded = false;
        snprintf(out_forecast->warning_message, sizeof(out_forecast->warning_message),
            "Memory utilization is projected to remain at %.1f%% within safe operational parameters.",
            reg_pred);
    }

    if (slope > 1e-6 && out_forecast->current_utilization_pct < threshold_pct) {
        double delta_pct = threshold_pct - out_forecast->current_utilization_pct;
        double sec = delta_pct / slope;
        out_forecast->seconds_to_exhaustion = (sec > 0 && sec < 86400) ? (int)sec : -1;
    }

    /* Generate fine-grained forecast points for frontend charting */
    int num_points = 8;
    out_forecast->point_count = num_points;
    uint64_t last_ts = samples[n - 1].timestamp;
    int step_sec = horizon_seconds / num_points;

    for (int i = 0; i < num_points; i++) {
        int off = (i + 1) * step_sec;
        double x_pt = last_x + off;
        double pred_y = (slope * x_pt) + intercept;
        if (pred_y < 0.0) pred_y = 0.0;
        if (pred_y > 100.0) pred_y = 100.0;

        out_forecast->points[i].timestamp = last_ts + off;
        out_forecast->points[i].offset_seconds = off;
        out_forecast->points[i].predicted_util_pct = pred_y;
        uint64_t used = (uint64_t)(out_forecast->total_ram_kb * (pred_y / 100.0));
        out_forecast->points[i].estimated_used_kb = used;
        out_forecast->points[i].estimated_available_kb = (out_forecast->total_ram_kb > used) ?
                                                         (out_forecast->total_ram_kb - used) : 0;
    }

    snprintf(out_forecast->limitation_explanation, sizeof(out_forecast->limitation_explanation),
        "Statistical moving average and linear regression extrapolate historical measurements. "
        "Sudden process launches, dynamic page cache evictions, or memory drops cannot be predicted.");

    return 0;
}

void mem_history_cleanup(void) {
    s_head = 0;
    s_count = 0;
}
