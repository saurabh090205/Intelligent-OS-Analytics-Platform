#ifndef TERMINAL_UI_H
#define TERMINAL_UI_H

#include <stdbool.h>

/* Launch the full interactive Terminal User Interface (TUI) */
int terminal_ui_run(void);

/* Individual terminal modules */
void tui_show_telemetry_dashboard(void);
void tui_show_process_monitor(void);
void tui_show_cpu_scheduling(void);
void tui_show_memory_analytics(void);
void tui_show_page_replacement(void);
void tui_show_deadlock_management(void);
void tui_show_log_analysis(void);
void tui_show_resource_advisory(void);
void tui_run_unit_tests(void);

#endif /* TERMINAL_UI_H */
