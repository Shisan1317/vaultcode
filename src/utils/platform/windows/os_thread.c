/**
 * @file    os_thread.c (Windows stub)
 * @brief   线程 — Windows CreateThread 桩实现
 * @author  RenJiaqi
 * @version 1.0.0
 * @date    2026-07-07
 *
 * @copyright Copyright (c) 2025-2026 VaultCode. All rights reserved.
 */

#include <stdlib.h>

#include "os_thread.h"

struct os_thread {
    int placeholder;
};

utl_err_t os_thread_create(os_thread_t **thread, os_thread_func_t func,
                           void *arg)
{
    UTL_UNUSED(thread);
    UTL_UNUSED(func);
    UTL_UNUSED(arg);
    return UTL_ERR_NOT_IMPL;
}

utl_err_t os_thread_join(os_thread_t *thread, void **retval)
{
    UTL_UNUSED(thread);
    UTL_UNUSED(retval);
    return UTL_ERR_NOT_IMPL;
}

utl_err_t os_thread_detach(os_thread_t *thread)
{
    UTL_UNUSED(thread);
    return UTL_ERR_NOT_IMPL;
}

void os_thread_destroy(os_thread_t *thread)
{
    UTL_UNUSED(thread);
}
