#include "server.h"
#include "routes.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>

#if defined(_WIN32)
#include <winsock2.h>
#include <ws2tcpip.h>
typedef int socklen_t;
#define CLOSE_SOCKET(s) closesocket(s)
#else
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#define CLOSE_SOCKET(s) close(s)
#define INVALID_SOCKET (-1)
#define SOCKET_ERROR   (-1)
typedef int SOCKET;
#endif

static volatile bool s_running = false;
static SOCKET s_listen_fd = INVALID_SOCKET;
static server_config_t s_config;

static void handle_sigint(int sig) {
    (void)sig;
    s_running = false;
    if (s_listen_fd != INVALID_SOCKET) {
        CLOSE_SOCKET(s_listen_fd);
        s_listen_fd = INVALID_SOCKET;
    }
}

static void send_response(SOCKET client_fd, const http_response_t *res) {
    char header_buf[1024];
    int header_len = snprintf(header_buf, sizeof(header_buf),
        "HTTP/1.1 %d %s\r\n"
        "Content-Type: %s; charset=utf-8\r\n"
        "Content-Length: %lu\r\n"
        "Access-Control-Allow-Origin: *\r\n"
        "Access-Control-Allow-Methods: GET, POST, OPTIONS\r\n"
        "Access-Control-Allow-Headers: Content-Type, Authorization\r\n"
        "Connection: close\r\n"
        "\r\n",
        res->status_code, res->status_text,
        res->content_type,
        (unsigned long)res->body_len);

    if (header_len > 0) {
        send(client_fd, header_buf, header_len, 0);
    }

    if (res->body && res->body_len > 0) {
        send(client_fd, res->body, (int)res->body_len, 0);
    }
}

static void process_client(SOCKET client_fd) {
    char req_buf[32768];
    int total_received = 0;
    int content_length = 0;
    char *header_end = NULL;

    /* Read until header boundary (\r\n\r\n or \n\n) */
    while (total_received < (int)sizeof(req_buf) - 1) {
        int r = recv(client_fd, req_buf + total_received, (int)sizeof(req_buf) - 1 - total_received, 0);
        if (r <= 0) break;
        total_received += r;
        req_buf[total_received] = '\0';

        header_end = strstr(req_buf, "\r\n\r\n");
        if (header_end) break;
        header_end = strstr(req_buf, "\n\n");
        if (header_end) break;
    }

    if (total_received <= 0 || !header_end) {
        CLOSE_SOCKET(client_fd);
        return;
    }

    /* Check for Content-Length */
    char *cl_ptr = strstr(req_buf, "Content-Length:");
    if (!cl_ptr) cl_ptr = strstr(req_buf, "content-length:");
    if (cl_ptr) {
        content_length = atoi(cl_ptr + 15);
    }

    int delim_len = (strncmp(header_end, "\r\n\r\n", 4) == 0) ? 4 : 2;
    int header_len = (int)(header_end - req_buf) + delim_len;
    int body_received = total_received - header_len;

    /* Read remaining body bytes if Content-Length > body_received */
    while (body_received < content_length && total_received < (int)sizeof(req_buf) - 1) {
        int needed = content_length - body_received;
        int max_read = (int)sizeof(req_buf) - 1 - total_received;
        if (needed > max_read) needed = max_read;
        int r = recv(client_fd, req_buf + total_received, needed, 0);
        if (r <= 0) break;
        total_received += r;
        body_received += r;
        req_buf[total_received] = '\0';
    }

    const char *body = (content_length > 0 && body_received > 0) ? (req_buf + header_len) : NULL;

    /* Parse request line */
    char method[16] = {0};
    char full_path[1024] = {0};
    char proto[16] = {0};

    char *first_line_end = strstr(req_buf, "\r\n");
    if (!first_line_end) first_line_end = strchr(req_buf, '\n');
    if (!first_line_end) {
        CLOSE_SOCKET(client_fd);
        return;
    }

    *first_line_end = '\0';
    if (sscanf(req_buf, "%15s %1023s %15s", method, full_path, proto) < 2) {
        CLOSE_SOCKET(client_fd);
        return;
    }

    /* Separate path and query string */
    char path[1024] = {0};
    char query[1024] = {0};
    char *qmark = strchr(full_path, '?');
    if (qmark) {
        size_t path_len = qmark - full_path;
        if (path_len >= sizeof(path)) path_len = sizeof(path) - 1;
        memcpy(path, full_path, path_len);
        path[path_len] = '\0';
        strncpy(query, qmark + 1, sizeof(query) - 1);
        query[sizeof(query) - 1] = '\0';
    } else {
        strncpy(path, full_path, sizeof(path) - 1);
        path[sizeof(path) - 1] = '\0';
    }

    http_response_t res;
    http_response_init(&res);

    handle_api_request(method, path, query, body, &res);

    send_response(client_fd, &res);

    http_response_free(&res);
    CLOSE_SOCKET(client_fd);
}

int server_init(const server_config_t *config) {
    if (config) {
        s_config = *config;
    } else {
        s_config.host = "127.0.0.1";
        s_config.port = 8080;
        s_config.backlog = 32;
        s_config.verbose = true;
    }

#if defined(_WIN32)
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        fprintf(stderr, "[server] WSAStartup failed\n");
        return -1;
    }
#endif

    s_listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (s_listen_fd == INVALID_SOCKET) {
        fprintf(stderr, "[server] Failed to create socket\n");
        return -1;
    }

    int opt = 1;
#if defined(_WIN32)
    setsockopt(s_listen_fd, SOL_SOCKET, SO_REUSEADDR, (const char *)&opt, sizeof(opt));
#else
    setsockopt(s_listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
#endif

    struct sockaddr_in saddr;
    memset(&saddr, 0, sizeof(saddr));
    saddr.sin_family = AF_INET;
    saddr.sin_port = htons((uint16_t)s_config.port);
    saddr.sin_addr.s_addr = inet_addr(s_config.host);

    if (bind(s_listen_fd, (struct sockaddr *)&saddr, sizeof(saddr)) == SOCKET_ERROR) {
        fprintf(stderr, "[server] Bind to %s:%d failed\n", s_config.host, s_config.port);
        CLOSE_SOCKET(s_listen_fd);
        s_listen_fd = INVALID_SOCKET;
        return -1;
    }

    if (listen(s_listen_fd, s_config.backlog) == SOCKET_ERROR) {
        fprintf(stderr, "[server] Listen failed\n");
        CLOSE_SOCKET(s_listen_fd);
        s_listen_fd = INVALID_SOCKET;
        return -1;
    }

    signal(SIGINT, handle_sigint);
    signal(SIGTERM, handle_sigint);

    printf("====================================================\n");
    printf(" Intelligent OS Analytics and Resource Manager\n");
    printf(" HTTP REST Server listening on http://%s:%d\n", s_config.host, s_config.port);
    printf(" Ready for frontend connections and API requests.\n");
    printf("====================================================\n");
    fflush(stdout);

    return 0;
}

int server_run(void) {
    if (s_listen_fd == INVALID_SOCKET) return -1;
    s_running = true;

    while (s_running) {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        SOCKET client_fd = accept(s_listen_fd, (struct sockaddr *)&client_addr, &client_len);

        if (client_fd == INVALID_SOCKET) {
            if (!s_running) break;
            continue;
        }

#if defined(_WIN32)
        DWORD timeout_ms = 4000;
        setsockopt(client_fd, SOL_SOCKET, SO_RCVTIMEO, (const char *)&timeout_ms, sizeof(timeout_ms));
        setsockopt(client_fd, SOL_SOCKET, SO_SNDTIMEO, (const char *)&timeout_ms, sizeof(timeout_ms));
#else
        struct timeval tv;
        tv.tv_sec = 4;
        tv.tv_usec = 0;
        setsockopt(client_fd, SOL_SOCKET, SO_RCVTIMEO, (const void *)&tv, sizeof(tv));
        setsockopt(client_fd, SOL_SOCKET, SO_SNDTIMEO, (const void *)&tv, sizeof(tv));
#endif

        process_client(client_fd);
    }

    return 0;
}

void server_stop(void) {
    s_running = false;
    if (s_listen_fd != INVALID_SOCKET) {
        CLOSE_SOCKET(s_listen_fd);
        s_listen_fd = INVALID_SOCKET;
    }
}

void server_cleanup(void) {
    server_stop();
#if defined(_WIN32)
    WSACleanup();
#endif
}
