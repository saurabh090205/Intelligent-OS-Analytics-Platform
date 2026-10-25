#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cpu_monitor.h"
#include "memory_monitor.h"
#include "process_monitor.h"
#include "memory_prediction.h"
#include "server.h"

static void print_usage(const char *prog_name) {
    printf("Intelligent OS Analytics and Resource Management Platform\n");
    printf("Backend C Engine - Phase 1: Process Monitoring & Metrics API\n\n");
    printf("Usage: %s [OPTIONS]\n", prog_name);
    printf("Options:\n");
    printf("  --port <port>     Set HTTP server port (default: 8080)\n");
    printf("  --host <ip>       Set HTTP server bind address (default: 127.0.0.1)\n");
    printf("  --help            Show this help message and exit\n");
    printf("\nEndpoints:\n");
    printf("  GET /api/health           - Backend health check & platform info\n");
    printf("  GET /api/system/overview  - Overall CPU, memory, process counters & uptime\n");
    printf("  GET /api/system/cpu       - CPU utilization, core stats, and model\n");
    printf("  GET /api/system/memory    - Memory details (RAM, swap, free, cached, buffers)\n");
    printf("  GET /api/processes        - Active accessible process list with CPU/memory\n");
    printf("  GET /api/processes/:pid   - Detailed stats for a single process\n");
}

int main(int argc, char *argv[]) {
    server_config_t config;
    config.host = "127.0.0.1";
    config.port = 8080;
    config.backlog = 64;
    config.verbose = true;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--port") == 0 && i + 1 < argc) {
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
