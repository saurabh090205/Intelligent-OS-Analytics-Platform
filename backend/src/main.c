#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cpu_monitor.h"
#include "memory_monitor.h"
#include "process_monitor.h"
#include "memory_prediction.h"
#include "server.h"
#include "terminal_ui.h"

static void print_usage(const char *prog_name) {
    printf("Intelligent OS Analytics and Resource Management Platform\n");
    printf("Full-Stack C Systems Engine (All Modules Active)\n\n");
    printf("Usage: %s [OPTIONS]\n", prog_name);
    printf("Options:\n");
    printf("  --cli, --tui, --terminal  Launch the Interactive Terminal UI (TUI) Dashboard\n");
    printf("  --server                  Start HTTP REST API server on default port 8080\n");
    printf("  --port <port>             Set HTTP server port (default: 8080)\n");
    printf("  --host <ip>               Set HTTP server bind address (default: 127.0.0.1)\n");
    printf("  --help                    Show this help message and exit\n");
    printf("\nREST API Endpoints:\n");
    printf("  GET  /api/health                      - Health check & platform info\n");
    printf("  GET  /api/system/overview             - CPU, memory, process counters & uptime\n");
    printf("  GET  /api/system/cpu                  - CPU utilization, core stats, model\n");
    printf("  GET  /api/system/memory               - RAM, swap, buffers, cached\n");
    printf("  GET  /api/processes                   - Live process table with CPU/memory\n");
    printf("  GET  /api/processes/:pid              - Single-process detail inspector\n");
    printf("  POST /api/scheduling/simulate         - FCFS, SJF, SRTF, Priority, RR + recommendation\n");
    printf("  GET  /api/scheduling/examples         - Preconfigured workload presets\n");
    printf("  GET  /api/memory/forecast             - OLS regression memory exhaustion forecast\n");
    printf("  GET  /api/memory/history              - Rolling utilization sample buffer\n");
    printf("  POST /api/page-replacement/simulate   - FIFO, LRU, Optimal simulation + Belady\n");
    printf("  GET  /api/page-replacement/examples   - Standard, Belady & locality presets\n");
    printf("  POST /api/deadlock/banker             - Banker's algorithm safety verification\n");
    printf("  POST /api/deadlock/request            - Resource request safety check\n");
    printf("  POST /api/deadlock/detect             - Circular deadlock cycle detector\n");
    printf("  GET  /api/deadlock/examples           - Textbook state presets\n");
    printf("  POST /api/logs/analyze                - Multi-format log parser & burst detector\n");
    printf("  GET  /api/logs/samples                - Sample syslog stream\n");
    printf("  GET  /api/advisory/recommendations    - Advisory resource allocation engine\n");
}

int main(int argc, char *argv[]) {
    server_config_t config;
    config.host = "127.0.0.1";
    config.port = 8080;
    config.backlog = 64;
    config.verbose = true;

    bool launch_tui = false;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--cli") == 0 || strcmp(argv[i], "--tui") == 0 || strcmp(argv[i], "--terminal") == 0) {
            launch_tui = true;
        } else if (strcmp(argv[i], "--server") == 0) {
            launch_tui = false;
        } else if (strcmp(argv[i], "--port") == 0 && i + 1 < argc) {
            config.port = atoi(argv[++i]);
        } else if (strcmp(argv[i], "--host") == 0 && i + 1 < argc) {
            config.host = argv[++i];
        } else if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0) {
            print_usage(argv[0]);
            return 0;
        } else {
            fprintf(stderr, "Unknown option: %s\n", argv[i]);
            print_usage(argv[0]);
            return 1;
        }
    }

    if (launch_tui) {
        return terminal_ui_run();
    }

    if (argc == 1) {
        printf("====================================================\n");
        printf(" Intelligent OS Analytics Platform\n");
        printf(" [1] Launch Terminal User Interface (TUI Dashboard)\n");
        printf(" [2] Start HTTP REST API Server (Port 8080)\n");
        printf("====================================================\n");
        printf("Select mode (1 or 2, default: 1): ");
        fflush(stdout);

        char line[16];
        if (fgets(line, sizeof(line), stdin)) {
            if (line[0] == '2') {
                /* Continue to server startup below */
            } else {
                return terminal_ui_run();
            }
        } else {
            return terminal_ui_run();
        }
    }

    /* Initialize monitoring subsystems */
    if (cpu_monitor_init() != 0) {
        fprintf(stderr, "[warn] CPU monitor initialization had warnings\n");
    }
    if (memory_monitor_init() != 0) {
        fprintf(stderr, "[warn] Memory monitor initialization had warnings\n");
    }
    if (process_monitor_init() != 0) {
        fprintf(stderr, "[warn] Process monitor initialization had warnings\n");
    }
    if (mem_history_init() != 0) {
        fprintf(stderr, "[warn] Memory prediction history initialization had warnings\n");
    }

    /* Initialize and run HTTP server */
    if (server_init(&config) != 0) {
        fprintf(stderr, "[error] Failed to initialize HTTP server on %s:%d\n", config.host, config.port);
        return 1;
    }

    server_run();

    /* Clean shutdown */
    printf("\nShutting down Intelligent OS Analytics Server...\n");
    server_cleanup();
    mem_history_cleanup();
    process_monitor_cleanup();
    memory_monitor_cleanup();
    cpu_monitor_cleanup();
    printf("Shutdown complete. Clean exit.\n");

    return 0;
}
