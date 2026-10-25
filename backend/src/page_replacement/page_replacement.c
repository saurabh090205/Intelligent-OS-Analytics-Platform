#define __USE_MINGW_ANSI_STDIO 1
#include "page_replacement.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

const char *page_algo_to_string(page_algo_t algo) {
    switch (algo) {
        case PAGE_ALGO_FIFO: return "First-In First-Out (FIFO)";
        case PAGE_ALGO_LRU: return "Least Recently Used (LRU)";
        case PAGE_ALGO_OPTIMAL: return "Optimal Page Replacement (OPT)";
        default: return "Unknown";
    }
}

/* 1. FIFO Simulation */
int run_page_fifo(const int *refs, int ref_count, int num_frames, page_sim_result_t *out_result) {
    if (!refs || ref_count <= 0 || num_frames <= 0 || !out_result) return -1;
    if (num_frames > MAX_FRAMES) num_frames = MAX_FRAMES;
    if (ref_count > MAX_PAGE_REFS) ref_count = MAX_PAGE_REFS;

    memset(out_result, 0, sizeof(page_sim_result_t));
    out_result->algo = PAGE_ALGO_FIFO;
    out_result->algo_name = page_algo_to_string(PAGE_ALGO_FIFO);
    out_result->frame_capacity = num_frames;
    out_result->total_references = ref_count;
    out_result->step_count = ref_count;

    int frames[MAX_FRAMES];
    for (int i = 0; i < num_frames; i++) frames[i] = -1;

    int fifo_ptr = 0;
    int hits = 0, faults = 0;

    for (int s = 0; s < ref_count; s++) {
        int page = refs[s];
        bool is_hit = false;
        int hit_frame_idx = -1;

        for (int f = 0; f < num_frames; f++) {
            if (frames[f] == page) {
                is_hit = true;
                hit_frame_idx = f;
                break;
            }
        }

        page_step_t *step = &out_result->steps[s];
        step->step = s;
        step->page = page;
        step->frame_count = num_frames;

        if (is_hit) {
            hits++;
            step->is_hit = true;
            step->victim_page = -1;
            step->replaced_frame_idx = hit_frame_idx;
        } else {
            faults++;
            step->is_hit = false;

            /* Check for empty frame first */
            int empty_slot = -1;
            for (int f = 0; f < num_frames; f++) {
                if (frames[f] == -1) {
                    empty_slot = f;
                    break;
                }
            }

            if (empty_slot != -1) {
                frames[empty_slot] = page;
                step->victim_page = -1;
                step->replaced_frame_idx = empty_slot;
            } else {
                int victim = frames[fifo_ptr];
                frames[fifo_ptr] = page;
                step->victim_page = victim;
                step->replaced_frame_idx = fifo_ptr;
                fifo_ptr = (fifo_ptr + 1) % num_frames;
            }
        }

        for (int f = 0; f < num_frames; f++) {
            step->frames[f] = frames[f];
        }
    }

    out_result->hit_count = hits;
    out_result->fault_count = faults;
    out_result->hit_ratio = (double)hits / ref_count;
    out_result->fault_ratio = (double)faults / ref_count;

    return 0;
}

/* 2. LRU Simulation */
int run_page_lru(const int *refs, int ref_count, int num_frames, page_sim_result_t *out_result) {
    if (!refs || ref_count <= 0 || num_frames <= 0 || !out_result) return -1;
    if (num_frames > MAX_FRAMES) num_frames = MAX_FRAMES;
    if (ref_count > MAX_PAGE_REFS) ref_count = MAX_PAGE_REFS;

    memset(out_result, 0, sizeof(page_sim_result_t));
    out_result->algo = PAGE_ALGO_LRU;
    out_result->algo_name = page_algo_to_string(PAGE_ALGO_LRU);
    out_result->frame_capacity = num_frames;
    out_result->total_references = ref_count;
    out_result->step_count = ref_count;

    int frames[MAX_FRAMES];
    int last_used[MAX_FRAMES];
    for (int i = 0; i < num_frames; i++) {
        frames[i] = -1;
        last_used[i] = -1;
    }

    int hits = 0, faults = 0;

    for (int s = 0; s < ref_count; s++) {
        int page = refs[s];
        bool is_hit = false;
        int hit_frame_idx = -1;

        for (int f = 0; f < num_frames; f++) {
            if (frames[f] == page) {
                is_hit = true;
                hit_frame_idx = f;
                break;
            }
        }

        page_step_t *step = &out_result->steps[s];
        step->step = s;
        step->page = page;
        step->frame_count = num_frames;

        if (is_hit) {
            hits++;
            step->is_hit = true;
            step->victim_page = -1;
            step->replaced_frame_idx = hit_frame_idx;
            last_used[hit_frame_idx] = s;
        } else {
            faults++;
            step->is_hit = false;

            int empty_slot = -1;
            for (int f = 0; f < num_frames; f++) {
                if (frames[f] == -1) {
                    empty_slot = f;
                    break;
                }
            }

            if (empty_slot != -1) {
                frames[empty_slot] = page;
                last_used[empty_slot] = s;
                step->victim_page = -1;
                step->replaced_frame_idx = empty_slot;
            } else {
                /* Find LRU frame (lowest last_used index) */
                int lru_idx = 0;
                int min_used = last_used[0];
                for (int f = 1; f < num_frames; f++) {
                    if (last_used[f] < min_used) {
                        min_used = last_used[f];
                        lru_idx = f;
                    }
                }

                int victim = frames[lru_idx];
                frames[lru_idx] = page;
                last_used[lru_idx] = s;
                step->victim_page = victim;
                step->replaced_frame_idx = lru_idx;
            }
        }

        for (int f = 0; f < num_frames; f++) {
            step->frames[f] = frames[f];
        }
    }

    out_result->hit_count = hits;
    out_result->fault_count = faults;
    out_result->hit_ratio = (double)hits / ref_count;
    out_result->fault_ratio = (double)faults / ref_count;

    return 0;
}

/* 3. Optimal Page Replacement (Belady's OPT / MIN) */
int run_page_optimal(const int *refs, int ref_count, int num_frames, page_sim_result_t *out_result) {
    if (!refs || ref_count <= 0 || num_frames <= 0 || !out_result) return -1;
    if (num_frames > MAX_FRAMES) num_frames = MAX_FRAMES;
    if (ref_count > MAX_PAGE_REFS) ref_count = MAX_PAGE_REFS;

    memset(out_result, 0, sizeof(page_sim_result_t));
    out_result->algo = PAGE_ALGO_OPTIMAL;
    out_result->algo_name = page_algo_to_string(PAGE_ALGO_OPTIMAL);
    out_result->frame_capacity = num_frames;
    out_result->total_references = ref_count;
    out_result->step_count = ref_count;

    int frames[MAX_FRAMES];
    for (int i = 0; i < num_frames; i++) frames[i] = -1;

    int hits = 0, faults = 0;

    for (int s = 0; s < ref_count; s++) {
        int page = refs[s];
        bool is_hit = false;
        int hit_frame_idx = -1;

        for (int f = 0; f < num_frames; f++) {
            if (frames[f] == page) {
                is_hit = true;
                hit_frame_idx = f;
                break;
            }
        }

        page_step_t *step = &out_result->steps[s];
        step->step = s;
        step->page = page;
        step->frame_count = num_frames;

        if (is_hit) {
            hits++;
            step->is_hit = true;
            step->victim_page = -1;
            step->replaced_frame_idx = hit_frame_idx;
        } else {
            faults++;
            step->is_hit = false;

            int empty_slot = -1;
            for (int f = 0; f < num_frames; f++) {
                if (frames[f] == -1) {
                    empty_slot = f;
                    break;
                }
            }

            if (empty_slot != -1) {
                frames[empty_slot] = page;
                step->victim_page = -1;
                step->replaced_frame_idx = empty_slot;
            } else {
                /* Find optimal victim: the one referenced farthest in the future or never again */
                int opt_idx = -1;
                int farthest = -1;

                for (int f = 0; f < num_frames; f++) {
                    int next_use = 1000000000;
                    for (int k = s + 1; k < ref_count; k++) {
                        if (refs[k] == frames[f]) {
                            next_use = k;
                            break;
                        }
                    }
                    if (next_use > farthest) {
                        farthest = next_use;
                        opt_idx = f;
                    }
                }

                if (opt_idx == -1) opt_idx = 0;
                int victim = frames[opt_idx];
                frames[opt_idx] = page;
                step->victim_page = victim;
                step->replaced_frame_idx = opt_idx;
            }
        }

        for (int f = 0; f < num_frames; f++) {
            step->frames[f] = frames[f];
        }
    }

    out_result->hit_count = hits;
    out_result->fault_count = faults;
    out_result->hit_ratio = (double)hits / ref_count;
    out_result->fault_ratio = (double)faults / ref_count;

    return 0;
}

/* Compare all three algorithms */
int run_all_page_replacement(const int *refs, int ref_count, int num_frames, page_comparison_t *out_comp) {
    if (!refs || ref_count <= 0 || num_frames <= 0 || !out_comp) return -1;
    memset(out_comp, 0, sizeof(page_comparison_t));

    run_page_fifo(refs, ref_count, num_frames, &out_comp->results[PAGE_ALGO_FIFO]);
    run_page_lru(refs, ref_count, num_frames, &out_comp->results[PAGE_ALGO_LRU]);
    run_page_optimal(refs, ref_count, num_frames, &out_comp->results[PAGE_ALGO_OPTIMAL]);

    int min_faults = out_comp->results[PAGE_ALGO_OPTIMAL].fault_count;
    page_algo_t best_algo = PAGE_ALGO_OPTIMAL;

    for (int a = 0; a < PAGE_ALGO_COUNT; a++) {
        if (out_comp->results[a].fault_count < min_faults) {
            min_faults = out_comp->results[a].fault_count;
            best_algo = (page_algo_t)a;
        }
    }

    out_comp->best_algo = best_algo;
    out_comp->best_algo_name = page_algo_to_string(best_algo);
    out_comp->min_faults = min_faults;

    snprintf(out_comp->comparison_summary, sizeof(out_comp->comparison_summary),
        "Optimal page replacement generated the minimum page faults (%d faults, %.1f%% hit ratio) serving as theoretical lower bound. "
        "LRU achieved %d faults (%.1f%% hits), outperforming FIFO (%d faults, %.1f%% hits).",
        out_comp->results[PAGE_ALGO_OPTIMAL].fault_count,
        out_comp->results[PAGE_ALGO_OPTIMAL].hit_ratio * 100.0,
        out_comp->results[PAGE_ALGO_LRU].fault_count,
        out_comp->results[PAGE_ALGO_LRU].hit_ratio * 100.0,
        out_comp->results[PAGE_ALGO_FIFO].fault_count,
        out_comp->results[PAGE_ALGO_FIFO].hit_ratio * 100.0);

    return 0;
}
