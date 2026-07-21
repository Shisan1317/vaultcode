/**
 * @file    os_time.c (Linux)
 * @brief   Time — POSIX clock_gettime implementation
 * @author  RenJiaqi
 * @version 1.0.0
 * @date    2026-07-07
 *
 * @copyright Copyright (c) 2025-2026 VaultCode. All rights reserved.
 */

#define _POSIX_C_SOURCE 199309L
#include <time.h>
#include <string.h>
#include <errno.h>

#include "os_time.h"

os_tick_t os_get_tick_ms(void)
{
    struct timespec ts;
    if (clock_gettime(CLOCK_MONOTONIC, &ts) != 0) {
        return 0;
    }
    return (os_tick_t)ts.tv_sec * 1000ULL + (os_tick_t)ts.tv_nsec / 1000000ULL;
}

void os_sleep_ms(uint32_t ms)
{
    struct timespec ts;
    ts.tv_sec  = ms / 1000;
    ts.tv_nsec = (ms % 1000) * 1000000L;

    /* nanosleep may be interrupted by signal, retry in loop */
    while (nanosleep(&ts, &ts) == -1 && errno == EINTR) {
        /* Continue sleeping the remaining time */
    }
}

utl_err_t os_get_time_str(char *buf, size_t size)
{
    if (buf == NULL || size == 0) {
        return UTL_ERR_NULL;
    }

    time_t now = time(NULL);
    struct tm tm_info;

    if (localtime_r(&now, &tm_info) == NULL) {
        return UTL_ERR_INTERNAL;
    }

    if (strftime(buf, size, "%Y-%m-%d %H:%M:%S", &tm_info) == 0) {
        buf[0] = '\0';
        return UTL_ERR_INTERNAL;
    }

    return UTL_OK;
}
