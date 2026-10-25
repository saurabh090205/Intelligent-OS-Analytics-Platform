#ifndef RESOURCE_ADVISORY_H
#define RESOURCE_ADVISORY_H

#include <stdbool.h>
#include <stdint.h>

#define MAX_ADVISORY_ITEMS 16

typedef enum {
    ADVISORY_CAT_CPU = 0,
    ADVISORY_CAT_MEMORY = 1,
    ADVISORY_CAT_PROCESS = 2,
    ADVISORY_CAT_PAGING = 3
} advisory_category_t;

typedef enum {
    ADVISORY_SEV_INFO = 0,
    ADVISORY_SEV_WARNING = 1,
    ADVISORY_SEV_CRITICAL = 2
} advisory_severity_t;

typedef struct {
    char id[32];
    advisory_category_t category;
    char category_name[24];
    advisory_severity_t severity;
    char severity_name[12];
    char title[96];
    char rationale[384];
    char recommended_action[384];
    int confidence_pct;
    bool is_pure_telemetry;
    char metric_name[48];
    char measured_value[32];
    char threshold_value[32];
} advisory_item_t;

typedef struct {
    char cpu_pressure[16];
    char memory_pressure[16];
    char process_pressure[16];
    char paging_pressure[16];
    char overall_health[24];
    int total_recommendations;
    int warning_count;
    int critical_count;
} resource_pressure_overview_t;

typedef struct {
    resource_pressure_overview_t overview;
    advisory_item_t items[MAX_ADVISORY_ITEMS];
    int item_count;
    char disclaimer[384];
} resource_advisory_report_t;

/* Functions */
int resource_advisory_generate(resource_advisory_report_t *report);

#endif /* RESOURCE_ADVISORY_H */
