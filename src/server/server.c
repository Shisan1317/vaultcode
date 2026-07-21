/**
 * @file    server.c
 * @brief   Unix Domain Socket JSON-RPC server implementation
 * @author  RenJiaqi
 * @version 1.0.0
 * @date    2026-07-07
 *
 * @copyright Copyright (c) 2025-2026 VaultCode. All rights reserved.
 *
 * Protocol: one JSON per line, \n delimited, request->response bidirectional
 */

#define _POSIX_C_SOURCE 200809L
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <errno.h>

#include "server.h"
#include "os_thread.h"
#include "os_log.h"

#define SERVER_MAX_CONN  16
#define SERVER_BUF_SIZE  65536

struct server {
    char              sock_path[256];
    server_handler_t  handler;
    int               fd;
    volatile bool     running;
};

/* ---- Minimal JSON extractor (no third-party dependencies) ---- */

/* Extract "key":"string_value" or "key":numeric_value from JSON */
static char *prv_json_get_str(const char *json, const char *key)
{
    char search[128];
    snprintf(search, sizeof(search), "\"%s\"", key);
    const char *pos = strstr(json, search);
    if (!pos) return NULL;

    pos += strlen(search);
    while (*pos == ' ' || *pos == ':' || *pos == '\t') pos++;

    /* Handle numeric values (e.g. "id":1) */
    if (*pos >= '0' && *pos <= '9') {
        const char *end = pos;
        while (*end >= '0' && *end <= '9') end++;
        size_t len = (size_t)(end - pos);
        char *val = (char *)malloc(len + 1);
        if (!val) return NULL;
        memcpy(val, pos, len);
        val[len] = '\0';
        return val;
    }

    if (*pos != '"') return NULL;
    pos++;

    const char *end = strchr(pos, '"');
    if (!end) return NULL;

    size_t len = (size_t)(end - pos);
    char *val = (char *)malloc(len + 1);
    if (!val) return NULL;
    memcpy(val, pos, len);
    val[len] = '\0';
    return val;
}

/* ---- Connection handler (one thread per connection) ---- */

typedef struct {
    int    client_fd;
    server_handler_t handler;
} conn_ctx_t;

static void *prv_handle_conn(void *arg)
{
    conn_ctx_t *ctx = (conn_ctx_t *)arg;
    char buf[SERVER_BUF_SIZE];

    FILE *stream = fdopen(ctx->client_fd, "r+");
    if (!stream) { close(ctx->client_fd); free(ctx); return NULL; }

    while (fgets(buf, sizeof(buf), stream) != NULL) {
        /* Strip trailing newline */
        size_t len = strlen(buf);
        while (len > 0 && (buf[len-1] == '\n' || buf[len-1] == '\r'))
            buf[--len] = '\0';
        if (len == 0) continue;

        /* Extract method, params, id */
        char *method = prv_json_get_str(buf, "method");
        char *id_str = prv_json_get_str(buf, "id");

        /* Extract params object (brace counting for nesting) */
        char *params = NULL;
        const char *ps = strstr(buf, "\"params\"");
        if (ps) {
            ps = strchr(ps, '{');
            if (ps) {
                int depth = 0;
                const char *pe = ps;
                while (*pe) {
                    if (*pe == '{') depth++;
                    else if (*pe == '}') { depth--; if (depth == 0) break; }
                    else if (*pe == '"') { pe++; while (*pe && *pe != '"') { if (*pe == '\\') pe++; pe++; } }
                    pe++;
                }
                if (depth == 0 && pe > ps) {
                    size_t plen = (size_t)(pe - ps + 1);
                    params = (char *)malloc(plen + 1);
                    if (params) { memcpy(params, ps, plen); params[plen] = '\0'; }
                }
            }
        }

        /* Call handler */
        char *result = ctx->handler(method ? method : "", params ? params : "{}",
                                    id_str ? id_str : "0");

        /* Write response */
        fprintf(stream, "%s\n", result);

        free(method); free(id_str); free(params); free(result);
    }

    fclose(stream);
    close(ctx->client_fd);
    free(ctx);
    return NULL;
}

/* ---- Public API ---- */

utl_err_t server_create(const char *socket_path, server_handler_t handler,
                        server_t **srv)
{
    if (!socket_path || !handler || !srv) return UTL_ERR_NULL;

    server_t *s = (server_t *)calloc(1, sizeof(server_t));
    if (!s) return UTL_ERR_MEM;

    strncpy(s->sock_path, socket_path, sizeof(s->sock_path) - 1);
    s->sock_path[sizeof(s->sock_path) - 1] = '\0';
    s->handler = handler;
    s->running  = false;

    /* Remove old socket file */
    unlink(s->sock_path);

    /* Create socket */
    s->fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (s->fd < 0) { free(s); return UTL_ERR_IO; }

    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, s->sock_path, sizeof(addr.sun_path) - 1);

    if (bind(s->fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        close(s->fd); free(s); return UTL_ERR_IO;
    }

    if (listen(s->fd, SERVER_MAX_CONN) < 0) {
        close(s->fd); unlink(s->sock_path); free(s);
        return UTL_ERR_IO;
    }

    *srv = s;
    return UTL_OK;
}

void server_destroy(server_t *srv)
{
    if (!srv) return;
    server_stop(srv);
    if (srv->fd >= 0) {
        close(srv->fd);
        unlink(srv->sock_path);
    }
    free(srv);
}

utl_err_t server_run(server_t *srv)
{
    if (!srv) return UTL_ERR_NULL;

    srv->running = true;
    OS_LOG_INFO("Server listening on %s", srv->sock_path);

    while (srv->running) {
        struct sockaddr_un client_addr;
        socklen_t addr_len = sizeof(client_addr);
        int client_fd = accept(srv->fd, (struct sockaddr *)&client_addr,
                               &addr_len);
        if (client_fd < 0) {
            if (errno == EINTR) continue;
            OS_LOG_ERROR("accept failed: %s", strerror(errno));
            break;
        }

        /* Spawn thread for new connection */
        conn_ctx_t *ctx = (conn_ctx_t *)malloc(sizeof(conn_ctx_t));
        if (!ctx) { close(client_fd); continue; }
        ctx->client_fd = client_fd;
        ctx->handler   = srv->handler;

        os_thread_t *th = NULL;
        os_thread_create(&th, prv_handle_conn, ctx);
        if (th) os_thread_detach(th); /* fire-and-forget */
    }

    return UTL_OK;
}

void server_stop(server_t *srv)
{
    if (!srv) return;
    srv->running = false;
}
