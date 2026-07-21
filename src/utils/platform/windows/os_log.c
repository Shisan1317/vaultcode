/**
 * @file    os_log.c (Windows stub)
 * @brief   日志 — Windows OutputDebugString 桩实现
 * @author  RenJiaqi
 * @version 1.0.0
 * @date    2026-07-07
 *
 * @copyright Copyright (c) 2025-2026 VaultCode. All rights reserved.
 */

#ifdef SUPPORT_UTL_SYS_LOG

#include <stdio.h>
#include <stdarg.h>

#include "os_log.h"

void os_log_write(const char *level, const char *file, int line,
                  const char *fmt, ...)
{
    UTL_UNUSED(level);
    UTL_UNUSED(file);
    UTL_UNUSED(line);

    /* 基础桩:仅输出到 stderr,无加锁保护 */
    va_list args;
    va_start(args, fmt);
    fprintf(stderr, "[%s] %s:%d — ", level, file, line);
    vfprintf(stderr, fmt, args);
    fprintf(stderr, "\n");
    va_end(args);
}

#endif /* SUPPORT_UTL_SYS_LOG */
