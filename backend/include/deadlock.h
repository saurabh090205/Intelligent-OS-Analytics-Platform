#ifndef DEADLOCK_H
#define DEADLOCK_H

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

#define MAX_DEADLOCK_PROCESSES 16
#define MAX_DEADLOCK_RESOURCES 8

/* Resource allocation model structures */
typedef struct {
    int num_processes;
    int num_resources;
    char process_names[MAX_DEADLOCK_PROCESSES][16];
    char resource_names[MAX_DEADLOCK_RESOURCES][16];

    int available[MAX_DEADLOCK_RESOURCES];
    int max_claim[MAX_DEADLOCK_PROCESSES][MAX_DEADLOCK_RESOURCES];
    int allocation[MAX_DEADLOCK_PROCESSES][MAX_DEADLOCK_RESOURCES];
    int need[MAX_DEADLOCK_PROCESSES][MAX_DEADLOCK_RESOURCES];
} deadlock_system_state_t;

/* Banker's algorithm safe-state analysis result */
typedef struct {
    bool is_safe;
    int safe_sequence[MAX_DEADLOCK_PROCESSES];
    int sequence_length;

    /* Step-by-step verification trace for explanation */
    int step_process[MAX_DEADLOCK_PROCESSES];
    int step_work[MAX_DEADLOCK_PROCESSES][MAX_DEADLOCK_RESOURCES];
    int step_count;

    char status_message[256];
    char explanation[512];
} bankers_result_t;

/* Request-grant test result */
typedef struct {
    int process_idx;
    int request[MAX_DEADLOCK_RESOURCES];
    bool can_grant;
    bool would_be_safe;
    char reason[256];
    bankers_result_t resulting_state;
} resource_request_result_t;

/* Deadlock detection result (for current allocation + request graph) */
typedef struct {
    bool is_deadlocked;
    int deadlocked_processes[MAX_DEADLOCK_PROCESSES];
    int deadlocked_count;
    char message[256];
    char explanation[512];
} deadlock_detection_result_t;

/* Preset scenario for demonstration */
typedef struct {
    const char *id;
    const char *title;
    const char *description;
    deadlock_system_state_t state;
} deadlock_example_t;

/* Core algorithm functions */
int deadlock_calculate_need(deadlock_system_state_t *state);
int deadlock_validate_state(const deadlock_system_state_t *state, char *err_buf, size_t err_size);

int bankers_safety_analysis(const deadlock_system_state_t *state, bankers_result_t *result);
int bankers_check_request(const deadlock_system_state_t *state, int process_idx, const int *request, resource_request_result_t *result);

int deadlock_detect(const deadlock_system_state_t *state, const int request_matrix[MAX_DEADLOCK_PROCESSES][MAX_DEADLOCK_RESOURCES], deadlock_detection_result_t *result);

int deadlock_get_example(const char *id, deadlock_system_state_t *out_state);

#endif /* DEADLOCK_H */
