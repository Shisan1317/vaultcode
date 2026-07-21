/**
 * @file    os_mutex.c (Windows stub)
 * @brief   互斥锁 — Windows CRITICAL_SECTION 桩实现
 * @author  RenJiaqi
 * @version 1.0.0
 * @date    2026-07-07
 *
 * @copyright Copyright (c) 2025-2026 VaultCode. All rights reserved.
 *
 * 说明: Windows 平台暂未实现,所有接口返回 UTL_ERR_NOT_IMPL。
 *       后续可基于 CRITICAL_SECTION 完成实现,无需修改上层代码。
 */

#include <stdlib.h>

#include "os_mutex.h"

struct os_mutex {
    int placeholder;
};

utl_err_t os_mutex_create(os_mutex_t **mutex)
{
    UTL_UNUSED(mutex);
    return UTL_ERR_NOT_IMPL;
}

utl_err_t os_mutex_lock(os_mutex_t *mutex)
{
    UTL_UNUSED(mutex);
    return UTL_ERR_NOT_IMPL;
}

utl_err_t os_mutex_unlock(os_mutex_t *mutex)
{
    UTL_UNUSED(mutex);
    return UTL_ERR_NOT_IMPL;
}

void os_mutex_destroy(os_mutex_t *mutex)
{
    UTL_UNUSED(mutex);
}
