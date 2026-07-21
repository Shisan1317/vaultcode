/**
 * @file    os_thread.c (Linux)
 * @brief   线程 — pthread 实现
 * @author  RenJiaqi
 * @version 1.0.0
 * @date    2026-07-07
 *
 * @copyright Copyright (c) 2025-2026 VaultCode. All rights reserved.
 */

#include <stdlib.h>
#include <pthread.h>

#include "os_thread.h"

struct os_thread {
    pthread_t   handle;
    utl_bool_t  joined;     /* 防止重复join */
};

utl_err_t os_thread_create(os_thread_t **thread, os_thread_func_t func,
                           void *arg)
{
    if (thread == NULL || func == NULL) {
        return UTL_ERR_NULL;
    }

    os_thread_t *t = (os_thread_t *)malloc(sizeof(os_thread_t));
    if (t == NULL) {
        return UTL_ERR_MEM;
    }

    t->joined = UTL_FALSE;

    int ret = pthread_create(&t->handle, NULL,
                             (void *(*)(void *))func, arg);
    if (ret != 0) {
        free(t);
        return UTL_ERR_MEM;
    }

    *thread = t;
    return UTL_OK;
}

utl_err_t os_thread_join(os_thread_t *thread, void **retval)
{
    if (thread == NULL) {
        return UTL_ERR_NULL;
    }

    if (thread->joined) {
        return UTL_OK;  /* 已join,幂等 */
    }

    int ret = pthread_join(thread->handle, retval);
    if (ret == 0) {
        thread->joined = UTL_TRUE;
    }
    return (ret == 0) ? UTL_OK : UTL_ERR_INTERNAL;
}

utl_err_t os_thread_detach(os_thread_t *thread)
{
    if (thread == NULL) {
        return UTL_ERR_NULL;
    }

    int ret = pthread_detach(thread->handle);
    return (ret == 0) ? UTL_OK : UTL_ERR_INTERNAL;
}

void os_thread_destroy(os_thread_t *thread)
{
    if (thread == NULL) {
        return;
    }
    free(thread);
}
