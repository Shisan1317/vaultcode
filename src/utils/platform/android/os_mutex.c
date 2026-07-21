/**
 * @file    os_mutex.c (Linux)
 * @brief   互斥锁 — pthread 实现
 * @author  RenJiaqi
 * @version 1.0.0
 * @date    2026-07-07
 *
 * @copyright Copyright (c) 2025-2026 VaultCode. All rights reserved.
 */

#include <stdlib.h>
#include <pthread.h>

#include "os_mutex.h"

struct os_mutex {
    pthread_mutex_t handle;
};

utl_err_t os_mutex_create(os_mutex_t **mutex)
{
    if (mutex == NULL) {
        return UTL_ERR_NULL;
    }

    os_mutex_t *m = (os_mutex_t *)malloc(sizeof(os_mutex_t));
    if (m == NULL) {
        return UTL_ERR_MEM;
    }

    int ret = pthread_mutex_init(&m->handle, NULL);
    if (ret != 0) {
        free(m);
        return UTL_ERR_LOCK;
    }

    *mutex = m;
    return UTL_OK;
}

utl_err_t os_mutex_lock(os_mutex_t *mutex)
{
    if (mutex == NULL) {
        return UTL_OK;  /* NULL锁退化为空操作 */
    }

    int ret = pthread_mutex_lock(&mutex->handle);
    return (ret == 0) ? UTL_OK : UTL_ERR_LOCK;
}

utl_err_t os_mutex_unlock(os_mutex_t *mutex)
{
    if (mutex == NULL) {
        return UTL_OK;
    }

    int ret = pthread_mutex_unlock(&mutex->handle);
    return (ret == 0) ? UTL_OK : UTL_ERR_LOCK;
}

void os_mutex_destroy(os_mutex_t *mutex)
{
    if (mutex == NULL) {
        return;
    }

    pthread_mutex_destroy(&mutex->handle);
    free(mutex);
}
