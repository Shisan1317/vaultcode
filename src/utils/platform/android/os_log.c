/**
 * @file    os_log.c (Linux)
 * @brief   日志 — fprintf + 时间戳实现
 * @author  RenJiaqi
 * @version 1.0.0
 * @date    2026-07-07
 *
 * @copyright Copyright (c) 2025-2026 VaultCode. All rights reserved.
 *
 * 说明:
 *   - 仅在定义了 SUPPORT_UTL_SYS_LOG 宏时编译此文件
 *   - 该宏由 SConscript 编译选项传入
 */

#define _POSIX_C_SOURCE 199309L

#ifdef SUPPORT_UTL_SYS_LOG

#include <stdio.h>
#include <stdarg.h>
#include <time.h>

#include "os_log.h"
#include "os_mutex.h"  /* 日志写入需要互斥 */

/* 全局日志锁(进程级单例) */
static os_mutex_t *g_log_mutex = NULL;

/* 模块加载时自动初始化(首次调用触发) */
static void prv_log_init_once(void)
{
    if (g_log_mutex == NULL) {
        os_mutex_create(&g_log_mutex);
    }
}

void os_log_write(const char *level, const char *file, int line,
                  const char *fmt, ...)
{
    prv_log_init_once();

    if (g_log_mutex != NULL) {
        os_mutex_lock(g_log_mutex);
    }

    /* 时间戳 */
    time_t now = time(NULL);
    struct tm tm_info;
    char time_buf[32];
    if (localtime_r(&now, &tm_info) != NULL) {
        strftime(time_buf, sizeof(time_buf), "%Y-%m-%d %H:%M:%S", &tm_info);
    } else {
        time_buf[0] = '\0';
    }

    fprintf(stderr, "[%s] [%s] %s:%d — ", level, time_buf, file, line);

    va_list args;
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);

    fprintf(stderr, "\n");
    fflush(stderr);

    if (g_log_mutex != NULL) {
        os_mutex_unlock(g_log_mutex);
    }
}

#endif /* SUPPORT_UTL_SYS_LOG */
