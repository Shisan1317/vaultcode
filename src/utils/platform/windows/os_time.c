/**
 * @file    os_time.c (Windows stub)
 * @brief   时间 — Windows QueryPerformanceCounter 桩实现
 * @author  RenJiaqi
 * @version 1.0.0
 * @date    2026-07-07
 *
 * @copyright Copyright (c) 2025-2026 VaultCode. All rights reserved.
 */

#include "os_time.h"

os_tick_t os_get_tick_ms(void)
{
    return 0;
}

void os_sleep_ms(uint32_t ms)
{
    UTL_UNUSED(ms);
}

utl_err_t os_get_time_str(char *buf, size_t size)
{
    UTL_UNUSED(buf);
    UTL_UNUSED(size);
    return UTL_ERR_NOT_IMPL;
}
