#include "deadlock.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

int deadlock_calculate_need(deadlock_system_state_t *state) {
    if (!state) return -1;
    for (int i = 0; i < state->num_processes; i++) {
        for (int j = 0; j < state->num_resources; j++) {
            state->need[i][j] = state->max_claim[i][j] - state->allocation[i][j];
            if (state->need[i][j] < 0) {
                state->need[i][j] = 0;
            }
        }
    }
    return 0;
}

int deadlock_validate_state(const deadlock_system_state_t *state, char *err_buf, size_t err_size) {
    if (!state) {
        if (err_buf) snprintf(err_buf, err_size, "Null state pointer");
        return -1;
    }

    if (state->num_processes <= 0 || state->num_processes > MAX_DEADLOCK_PROCESSES) {
        if (err_buf) snprintf(err_buf, err_size, "Process count (%d) must be between 1 and %d", state->num_processes, MAX_DEADLOCK_PROCESSES);
        return -1;
    }

    if (state->num_resources <= 0 || state->num_resources > MAX_DEADLOCK_RESOURCES) {
        if (err_buf) snprintf(err_buf, err_size, "Resource count (%d) must be between 1 and %d", state->num_resources, MAX_DEADLOCK_RESOURCES);
        return -1;
    }

    for (int j = 0; j < state->num_resources; j++) {
        if (state->available[j] < 0) {
            if (err_buf) snprintf(err_buf, err_size, "Available resource %d cannot be negative (%d)", j, state->available[j]);
            return -1;
        }
    }

    for (int i = 0; i < state->num_processes; i++) {
        for (int j = 0; j < state->num_resources; j++) {
            if (state->allocation[i][j] < 0) {
                if (err_buf) snprintf(err_buf, err_size, "Allocation for P%d, R%d cannot be negative (%d)", i, j, state->allocation[i][j]);
                return -1;
            }
            if (state->max_claim[i][j] < 0) {
                if (err_buf) snprintf(err_buf, err_size, "Max claim for P%d, R%d cannot be negative (%d)", i, j, state->max_claim[i][j]);
                return -1;
            }
            if (state->allocation[i][j] > state->max_claim[i][j]) {
                if (err_buf) snprintf(err_buf, err_size, "Allocation (%d) exceeds Max claim (%d) for P%d, R%d", state->allocation[i][j], state->max_claim[i][j], i, j);
                return -1;
            }
        }
    }

    return 0;
}

int bankers_safety_analysis(const deadlock_system_state_t *state, bankers_result_t *result) {
    if (!state || !result) return -1;
    memset(result, 0, sizeof(bankers_result_t));

    int work[MAX_DEADLOCK_RESOURCES];
    bool finish[MAX_DEADLOCK_PROCESSES];

    for (int j = 0; j < state->num_resources; j++) {
        work[j] = state->available[j];
    }
    for (int i = 0; i < state->num_processes; i++) {
        finish[i] = false;
    }

    int seq_len = 0;
    bool found = true;

    while (found && seq_len < state->num_processes) {
        found = false;
        for (int i = 0; i < state->num_processes; i++) {
            if (!finish[i]) {
                bool can_satisfy = true;
                for (int j = 0; j < state->num_resources; j++) {
                    int needed = state->max_claim[i][j] - state->allocation[i][j];
                    if (needed < 0) needed = 0;
                    if (needed > work[j]) {
                        can_satisfy = false;
                        break;
                    }
                }

                if (can_satisfy) {
                    /* Allocate resources to process, let it finish and reclaim */
                    for (int j = 0; j < state->num_resources; j++) {
                        work[j] += state->allocation[i][j];
                        result->step_work[seq_len][j] = work[j];
                    }
                    result->step_process[seq_len] = i;
                    result->safe_sequence[seq_len] = i;
                    finish[i] = true;
                    seq_len++;
                    found = true;
                    break;
                }
            }
        }
    }

    result->sequence_length = seq_len;
    result->step_count = seq_len;

    if (seq_len == state->num_processes) {
        result->is_safe = true;
        snprintf(result->status_message, sizeof(result->status_message), "System is in a SAFE state.");

        char seq_str[256] = "";
        for (int k = 0; k < seq_len; k++) {
            char p_buf[32];
            int p_idx = result->safe_sequence[k];
            const char *p_name = (state->process_names[p_idx][0] != '\0') ? state->process_names[p_idx] : "P";
            snprintf(p_buf, sizeof(p_buf), "%s%s", p_name, (k < seq_len - 1) ? " -> " : "");
            strncat(seq_str, p_buf, sizeof(seq_str) - strlen(seq_str) - 1);
        }

        snprintf(result->explanation, sizeof(result->explanation),
                 "A valid execution sequence exists: <%s>. "
                 "All processes can satisfy their maximum resource claims and terminate without entering deadlock.",
                 seq_str);
    } else {
        result->is_safe = false;
        snprintf(result->status_message, sizeof(result->status_message), "System is in an UNSAFE state.");

        char blocked_procs[128] = "";
        for (int i = 0; i < state->num_processes; i++) {
            if (!finish[i]) {
                char p_buf[16];
                const char *p_name = (state->process_names[i][0] != '\0') ? state->process_names[i] : "P";
                snprintf(p_buf, sizeof(p_buf), "%s ", p_name);
                strncat(blocked_procs, p_buf, sizeof(blocked_procs) - strlen(blocked_procs) - 1);
            }
        }

        snprintf(result->explanation, sizeof(result->explanation),
                 "No safe sequence exists. Blocked processes: [%s]. "
                 "IMPORTANT: An unsafe state is NOT synonymous with deadlock. "
                 "An unsafe state means deadlock CANNOT be guaranteed against if processes claim their maximum declared demands. "
                 "Actual deadlock only occurs if processes simultaneously request remaining claims that exceed availability.",
                 blocked_procs);
    }

    return 0;
}

int bankers_check_request(const deadlock_system_state_t *state, int process_idx, const int *request, resource_request_result_t *result) {
    if (!state || !request || !result) return -1;
    memset(result, 0, sizeof(resource_request_result_t));

    result->process_idx = process_idx;
    for (int j = 0; j < state->num_resources; j++) {
        result->request[j] = request[j];
    }

    if (process_idx < 0 || process_idx >= state->num_processes) {
        result->can_grant = false;
        result->would_be_safe = false;
        snprintf(result->reason, sizeof(result->reason), "Invalid process index %d.", process_idx);
        return 0;
    }

    /* Condition 1: Request <= Need */
    for (int j = 0; j < state->num_resources; j++) {
        int needed = state->max_claim[process_idx][j] - state->allocation[process_idx][j];
        if (needed < 0) needed = 0;
        if (request[j] > needed) {
            result->can_grant = false;
            result->would_be_safe = false;
            snprintf(result->reason, sizeof(result->reason),
                     "Error: Process requested %d units of R%d, exceeding its declared remaining need (%d).",
                     request[j], j, needed);
            return 0;
        }
    }

    /* Condition 2: Request <= Available */
    for (int j = 0; j < state->num_resources; j++) {
        if (request[j] > state->available[j]) {
            result->can_grant = false;
            result->would_be_safe = false;
            snprintf(result->reason, sizeof(result->reason),
                     "Wait: Process must wait because requested %d units of R%d exceeds currently available units (%d).",
                     request[j], j, state->available[j]);
            return 0;
        }
    }

    /* Pretend to allocate and test safety */
    deadlock_system_state_t temp_state = *state;
    for (int j = 0; j < state->num_resources; j++) {
        temp_state.available[j] -= request[j];
        temp_state.allocation[process_idx][j] += request[j];
        temp_state.need[process_idx][j] = temp_state.max_claim[process_idx][j] - temp_state.allocation[process_idx][j];
    }

    bankers_result_t safety;
    bankers_safety_analysis(&temp_state, &safety);
    result->resulting_state = safety;

    if (safety.is_safe) {
        result->can_grant = true;
        result->would_be_safe = true;
        snprintf(result->reason, sizeof(result->reason),
                 "Request can be GRANTED immediately. Resulting system state remains in a SAFE state.");
    } else {
        result->can_grant = false;
        result->would_be_safe = false;
        snprintf(result->reason, sizeof(result->reason),
                 "Request DENIED. Granting this request would transition the system into an UNSAFE state. Process must wait.");
    }

    return 0;
}

int deadlock_detect(const deadlock_system_state_t *state, const int request_matrix[MAX_DEADLOCK_PROCESSES][MAX_DEADLOCK_RESOURCES], deadlock_detection_result_t *result) {
    if (!state || !result) return -1;
    memset(result, 0, sizeof(deadlock_detection_result_t));

    int work[MAX_DEADLOCK_RESOURCES];
    bool finish[MAX_DEADLOCK_PROCESSES];

    for (int j = 0; j < state->num_resources; j++) {
        work[j] = state->available[j];
    }

    /* In standard deadlock detection:
       Finish[i] = true if Allocation[i] == 0, else false */
    for (int i = 0; i < state->num_processes; i++) {
        bool has_alloc = false;
        for (int j = 0; j < state->num_resources; j++) {
            if (state->allocation[i][j] > 0) {
                has_alloc = true;
                break;
            }
        }
        finish[i] = !has_alloc;
    }

    bool found = true;
    while (found) {
        found = false;
        for (int i = 0; i < state->num_processes; i++) {
            if (!finish[i]) {
                bool can_satisfy = true;
                for (int j = 0; j < state->num_resources; j++) {
                    int req_val = request_matrix ? request_matrix[i][j] : (state->max_claim[i][j] - state->allocation[i][j]);
                    if (req_val < 0) req_val = 0;
                    if (req_val > work[j]) {
                        can_satisfy = false;
                        break;
                    }
                }

                if (can_satisfy) {
                    for (int j = 0; j < state->num_resources; j++) {
                        work[j] += state->allocation[i][j];
                    }
                    finish[i] = true;
                    found = true;
                    break;
                }
            }
        }
    }

    int dead_count = 0;
    char dead_names[256] = "";
    for (int i = 0; i < state->num_processes; i++) {
        if (!finish[i]) {
            result->deadlocked_processes[dead_count++] = i;
            char p_buf[16];
            const char *p_name = (state->process_names[i][0] != '\0') ? state->process_names[i] : "P";
            snprintf(p_buf, sizeof(p_buf), "%s ", p_name);
            strncat(dead_names, p_buf, sizeof(dead_names) - strlen(dead_names) - 1);
        }
    }

    result->deadlocked_count = dead_count;
    if (dead_count > 0) {
        result->is_deadlocked = true;
        snprintf(result->message, sizeof(result->message), "DEADLOCK DETECTED involving %d processes.", dead_count);
        snprintf(result->explanation, sizeof(result->explanation),
                 "A circular wait or unfulfillable request cycle exists among processes: [%s]. "
                 "These processes hold resources that other processes require, with available resources insufficient to fulfill any request.",
                 dead_names);
    } else {
        result->is_deadlocked = false;
        snprintf(result->message, sizeof(result->message), "No deadlock detected. All processes can complete.");
        snprintf(result->explanation, sizeof(result->explanation),
                 "The resource-allocation graph contains no unresolved resource request cycles. All processes can eventually release their held allocations.");
    }

    return 0;
}

int deadlock_get_example(const char *id, deadlock_system_state_t *out_state) {
    if (!id || !out_state) return -1;
    memset(out_state, 0, sizeof(deadlock_system_state_t));

    if (strcmp(id, "silberschatz_safe") == 0) {
        /* Standard Textbook Example (Silberschatz OS Concepts 10th Ed) */
        out_state->num_processes = 5;
        out_state->num_resources = 3;

        snprintf(out_state->process_names[0], 16, "P0");
        snprintf(out_state->process_names[1], 16, "P1");
        snprintf(out_state->process_names[2], 16, "P2");
        snprintf(out_state->process_names[3], 16, "P3");
        snprintf(out_state->process_names[4], 16, "P4");

        snprintf(out_state->resource_names[0], 16, "A (CPU)");
        snprintf(out_state->resource_names[1], 16, "B (Memory)");
        snprintf(out_state->resource_names[2], 16, "C (Disk)");

        /* Available = [3, 3, 2] */
        out_state->available[0] = 3;
        out_state->available[1] = 3;
        out_state->available[2] = 2;

        /* Allocation */
        int alloc[5][3] = {
            {0, 1, 0}, /* P0 */
            {2, 0, 0}, /* P1 */
            {3, 0, 2}, /* P2 */
            {2, 1, 1}, /* P3 */
            {0, 0, 2}  /* P4 */
        };

        /* Max Claim */
        int max[5][3] = {
            {7, 5, 3}, /* P0 */
            {3, 2, 2}, /* P1 */
            {9, 0, 2}, /* P2 */
            {2, 2, 2}, /* P3 */
            {4, 3, 3}  /* P4 */
        };

        for (int i = 0; i < 5; i++) {
            for (int j = 0; j < 3; j++) {
                out_state->allocation[i][j] = alloc[i][j];
                out_state->max_claim[i][j] = max[i][j];
            }
        }
        deadlock_calculate_need(out_state);
        return 0;
    }

    if (strcmp(id, "unsafe_state") == 0) {
        /* Example where Available is too low, creating an unsafe state without immediate deadlock */
        out_state->num_processes = 4;
        out_state->num_resources = 3;

        snprintf(out_state->process_names[0], 16, "P0");
        snprintf(out_state->process_names[1], 16, "P1");
        snprintf(out_state->process_names[2], 16, "P2");
        snprintf(out_state->process_names[3], 16, "P3");

        snprintf(out_state->resource_names[0], 16, "R0");
        snprintf(out_state->resource_names[1], 16, "R1");
        snprintf(out_state->resource_names[2], 16, "R2");

        out_state->available[0] = 1;
        out_state->available[1] = 0;
        out_state->available[2] = 0;

        int alloc[4][3] = {
            {2, 0, 0},
            {1, 1, 2},
            {2, 1, 1},
            {0, 0, 2}
        };

        int max[4][3] = {
            {4, 2, 2},
            {2, 2, 2},
            {4, 3, 3},
            {2, 1, 3}
        };

        for (int i = 0; i < 4; i++) {
            for (int j = 0; j < 3; j++) {
                out_state->allocation[i][j] = alloc[i][j];
                out_state->max_claim[i][j] = max[i][j];
            }
        }
        deadlock_calculate_need(out_state);
        return 0;
    }

    if (strcmp(id, "deadlock_cycle") == 0) {
        /* Classic Resource-Allocation Graph Deadlock Cycle: P0 holds R0 wants R1, P1 holds R1 wants R0 */
        out_state->num_processes = 3;
        out_state->num_resources = 2;

        snprintf(out_state->process_names[0], 16, "Proc-A");
        snprintf(out_state->process_names[1], 16, "Proc-B");
        snprintf(out_state->process_names[2], 16, "Proc-C");

        snprintf(out_state->resource_names[0], 16, "Mutex-1");
        snprintf(out_state->resource_names[1], 16, "Mutex-2");

        out_state->available[0] = 0;
        out_state->available[1] = 0;

        int alloc[3][2] = {
            {1, 0}, /* Proc-A holds Mutex-1 */
            {0, 1}, /* Proc-B holds Mutex-2 */
            {0, 0}
        };

        int max[3][2] = {
            {1, 1}, /* Proc-A needs Mutex-2 */
            {1, 1}, /* Proc-B needs Mutex-1 */
            {1, 0}
        };

        for (int i = 0; i < 3; i++) {
            for (int j = 0; j < 2; j++) {
                out_state->allocation[i][j] = alloc[i][j];
                out_state->max_claim[i][j] = max[i][j];
            }
        }
        deadlock_calculate_need(out_state);
        return 0;
    }

    return -1;
}
