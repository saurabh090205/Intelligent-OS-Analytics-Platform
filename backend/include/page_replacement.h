#ifndef PAGE_REPLACEMENT_H
#define PAGE_REPLACEMENT_H

#include <stdint.h>
#include <stdbool.h>

#define MAX_PAGE_REFS 256
#define MAX_FRAMES 16

typedef enum {
    PAGE_ALGO_FIFO = 0,
    PAGE_ALGO_LRU,
    PAGE_ALGO_OPTIMAL,
    PAGE_ALGO_COUNT
} page_algo_t;

typedef struct {
    int step;
    int page;
    bool is_hit;
    int frames[MAX_FRAMES];
    int frame_count;
    int victim_page;      /* -1 if no eviction or empty slot filled */
    int replaced_frame_idx;
} page_step_t;

typedef struct {
    page_algo_t algo;
    const char *algo_name;
    int frame_capacity;
    int total_references;
    int hit_count;
    int fault_count;
    double hit_ratio;
    double fault_ratio;
    page_step_t steps[MAX_PAGE_REFS];
    int step_count;
} page_sim_result_t;

typedef struct {
    page_sim_result_t results[PAGE_ALGO_COUNT];
    int best_algo;
    const char *best_algo_name;
    int min_faults;
    char comparison_summary[512];
    bool belady_detected;
} page_comparison_t;

int run_page_fifo(const int *refs, int ref_count, int num_frames, page_sim_result_t *out_result);
int run_page_lru(const int *refs, int ref_count, int num_frames, page_sim_result_t *out_result);
int run_page_optimal(const int *refs, int ref_count, int num_frames, page_sim_result_t *out_result);

int run_all_page_replacement(const int *refs, int ref_count, int num_frames, page_comparison_t *out_comp);

const char *page_algo_to_string(page_algo_t algo);

#endif /* PAGE_REPLACEMENT_H */
