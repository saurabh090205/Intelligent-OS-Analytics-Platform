#ifndef MEMORY_PREDICTION_H
#define MEMORY_PREDICTION_H

#include <stdint.h>
#include <stdbool.h>

#define MAX_MEM_HISTORY 256
#define MAX_FORECAST_POINTS 64

typedef struct {
    uint64_t timestamp;
    double utilization_pct;
    uint64_t used_kb;
    uint64_t available_kb;
    uint64_t total_kb;
} mem_sample_t;

typedef enum {
    MEM_TREND_DECREASING = -1,
    MEM_TREND_STABLE = 0,
    MEM_TREND_INCREASING = 1
} mem_trend_t;

typedef struct {
    uint64_t timestamp;
    int offset_seconds;
    double predicted_util_pct;
    uint64_t estimated_available_kb;
    uint64_t estimated_used_kb;
} mem_forecast_point_t;

typedef struct {
    mem_trend_t trend;
    const char *trend_label;
    double slope_pct_per_min;
    double r_squared;
    double current_utilization_pct;
    double predicted_sma_util_pct;
    double predicted_reg_util_pct;
    uint64_t current_available_kb;
    uint64_t predicted_available_kb;
    uint64_t total_ram_kb;
    int forecast_horizon_seconds;
    int sample_count;
    bool threshold_exceeded;
    double threshold_pct;
    int seconds_to_exhaustion; /* -1 if not exhausting */
    char warning_message[256];
    char limitation_explanation[256];
    mem_forecast_point_t points[MAX_FORECAST_POINTS];
    int point_count;
} mem_forecast_result_t;

int mem_history_init(void);
int mem_history_record_sample(double util_pct, uint64_t used_kb, uint64_t avail_kb, uint64_t total_kb);
int mem_history_get_samples(mem_sample_t *out_samples, int max_count, int *out_count);
int mem_predict_forecast(int horizon_seconds, int window_size, double threshold_pct,
                         mem_forecast_result_t *out_forecast);
void mem_history_cleanup(void);

#endif /* MEMORY_PREDICTION_H */
