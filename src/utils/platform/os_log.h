/**
 * @file    os_log.h
 * @brief   Cross-platform log output abstraction interface — cross-project reusable
 * @author  RenJiaqi
 * @version 1.0.0
 * @date    2026-07-07
 *
 * @copyright Copyright (c) 2025-2026 VaultCode. All rights reserved.
 *
 * Description:
 *   - Provides unified leveled log macros, controlled by compile-time switch SUPPORT_UTL_SYS_LOG
 *   - When SUPPORT_UTL_SYS_LOG is not defined, log macros compile to no-ops (zero overhead)
 *   - Log format: "[LEVEL] file:line — message"
 */

#ifndef OS_LOG_H
#define OS_LOG_H

#include "utl_config.h"
#include <stdio.h>

/*===========================================================================
 * Log levels
 *===========================================================================*/

#define OS_LOG_LEVEL_ERROR   0
#define OS_LOG_LEVEL_WARN    1
#define OS_LOG_LEVEL_INFO    2
#define OS_LOG_LEVEL_DEBUG   3

/*===========================================================================
 * Conditional compilation: SUPPORT_UTL_SYS_LOG switch
 *===========================================================================*/

#ifdef SUPPORT_UTL_SYS_LOG

/**
 * @brief Low-level log output function (do not call directly, use macros)
 * @param level  Level string: "ERROR"/"WARN "/"INFO "/"DEBUG"
 * @param file   Source file name
 * @param line   Line number
 * @param fmt    printf format string
 * @param ...    Variable arguments
 */
void os_log_write(const char *level, const char *file, int line,
                  const char *fmt, ...)
    UTL_PRINTF_LIKE(4, 5);

#define OS_LOG_ERROR(fmt, ...) \
    os_log_write("ERROR", __FILE__, __LINE__, fmt, ##__VA_ARGS__)

#define OS_LOG_WARN(fmt, ...) \
    os_log_write("WARN ", __FILE__, __LINE__, fmt, ##__VA_ARGS__)

#define OS_LOG_INFO(fmt, ...) \
    os_log_write("INFO ", __FILE__, __LINE__, fmt, ##__VA_ARGS__)

#define OS_LOG_DEBUG(fmt, ...) \
    os_log_write("DEBUG", __FILE__, __LINE__, fmt, ##__VA_ARGS__)

#else  /* !SUPPORT_UTL_SYS_LOG — log compiles to no-op */

#define OS_LOG_ERROR(fmt, ...)  ((void)0)
#define OS_LOG_WARN(fmt, ...)   ((void)0)
#define OS_LOG_INFO(fmt, ...)   ((void)0)
#define OS_LOG_DEBUG(fmt, ...)  ((void)0)

#endif /* SUPPORT_UTL_SYS_LOG */

#endif /* OS_LOG_H */
