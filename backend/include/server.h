#ifndef SERVER_H
#define SERVER_H

#include <stdbool.h>

typedef struct {
    const char *host;
    int port;
    int backlog;
    bool verbose;
} server_config_t;

int server_init(const server_config_t *config);
int server_run(void);
void server_stop(void);
void server_cleanup(void);

#endif /* SERVER_H */
