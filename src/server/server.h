/**
 * @file    server.h
 * @brief   JSON-RPC 2.0 Server over Unix Domain Socket
 * @author  RenJiaqi
 * @version 1.0.0
 * @date    2026-07-07
 *
 * @copyright Copyright (c) 2025-2026 VaultCode. All rights reserved.
 */

#ifndef SERVER_H
#define SERVER_H

#include "utl_types.h"
#include "utl_err.h"
#include "utl_config.h"

UTL_EXTERN_C_BEGIN

typedef struct server server_t;

/* Request handler callback: returns JSON response string (must be freed by caller) */
typedef char *(*server_handler_t)(const char *method, const char *params,
                                  const char *id_str);

utl_err_t server_create(const char *socket_path, server_handler_t handler,
                        server_t **srv);
void      server_destroy(server_t *srv);

/// @brief Start server loop (blocking)
utl_err_t server_run(server_t *srv);

/// @brief Stop server
void      server_stop(server_t *srv);

UTL_EXTERN_C_END

#endif /* SERVER_H */
