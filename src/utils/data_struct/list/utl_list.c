/**
 * @file    utl_list.c
 * @brief   General-purpose doubly-linked list module — object-oriented encapsulation implementation
 * @author  RenJiaqi
 * @version 2.0.0
 * @date    2026-07-07
 *
 * @copyright Copyright (c) 2025-2026 VaultCode. All rights reserved.
 *
 * Changelog:
 *   v1.0.0  2025-06-24  RenJiaqi  Initial version: basic doubly-linked list CRUD
 *   v2.0.0  2026-07-07  RenJiaqi  OOP refactoring, see header changelog for details
 */

#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "utl_list.h"
#include "os_mutex.h"

/*===========================================================================
 * Internal data structures (fully private, invisible externally)
 *===========================================================================*/

/// @brief List node — doubly-linked pointer + user data
typedef struct utl_list_node {
    void                *data;   ///< User data pointer
    struct utl_list_node *prev;  ///< Previous node
    struct utl_list_node *next;  ///< Next node
} utl_list_node_t;

/// @brief List control block — encapsulates head/tail/lock/count/capacity, external code only holds opaque pointer
struct utl_list_obj {
    utl_list_node_t *head;        ///< Head node
    utl_list_node_t *tail;        ///< Tail node
    size_t           size;        ///< Current node count
    uint32_t         max_size;    ///< Capacity upper limit (0 = unlimited)
    bool             thread_safe; ///< Thread-safe switch
    os_mutex_t      *mutex;       ///< Mutex handle (only valid when thread_safe = true)
};

/*===========================================================================
 * Default configuration
 *===========================================================================*/

static const utl_list_cfg_t kDefaultCfg = {
    .thread_safe = false,
    .max_size    = 0,
};

/*===========================================================================
 * Internal helper macros & functions
 *===========================================================================*/

/// @brief Lock (only effective in thread_safe mode)
static inline int prv_lock(utl_list_t *list)
{
    if (list->thread_safe && list->mutex != NULL) {
        return os_mutex_lock(list->mutex);
    }
    return 0;
}

/// @brief Unlock (only effective in thread_safe mode)
static inline int prv_unlock(utl_list_t *list)
{
    if (list->thread_safe && list->mutex != NULL) {
        return os_mutex_unlock(list->mutex);
    }
    return 0;
}

/// @brief Validate parameter validity
static inline bool prv_param_invalid(const void *ptr)
{
    return (ptr == NULL);
}

/// @brief Check if capacity limit has been reached
static inline bool prv_is_full(const utl_list_t *list)
{
    return (list->max_size > 0 && list->size >= list->max_size);
}

/// @brief Check if list is empty
static inline bool prv_is_empty(const utl_list_t *list)
{
    return (list->size == 0);
}

/// @brief Allocate a new node
static utl_list_node_t *prv_node_alloc(void *data)
{
    utl_list_node_t *node = (utl_list_node_t *)malloc(sizeof(utl_list_node_t));
    if (node != NULL) {
        node->data = data;
        node->prev = NULL;
        node->next = NULL;
    }
    return node;
}

/// @brief Free a single node
static inline void prv_node_free(utl_list_node_t *node)
{
    free(node);
}

/// @brief Link a new node to the tail of the list (caller must have locked and verified not full)
static void prv_link_back(utl_list_t *list, utl_list_node_t *node)
{
    node->prev = list->tail;
    node->next = NULL;

    if (list->tail != NULL) {
        list->tail->next = node;
    } else {
        list->head = node;
    }
    list->tail = node;
    list->size++;
}

/// @brief Link a new node to the head of the list (caller must have locked and verified not full)
static void prv_link_front(utl_list_t *list, utl_list_node_t *node)
{
    node->prev = NULL;
    node->next = list->head;

    if (list->head != NULL) {
        list->head->prev = node;
    } else {
        list->tail = node;
    }
    list->head = node;
    list->size++;
}

/// @brief Unlink and return the tail node from the list (caller must have locked and verified not empty)
static utl_list_node_t *prv_unlink_back(utl_list_t *list)
{
    utl_list_node_t *node = list->tail;
    list->tail = node->prev;

    if (list->tail != NULL) {
        list->tail->next = NULL;
    } else {
        list->head = NULL;
    }
    list->size--;
    return node;
}

/// @brief Unlink and return the head node from the list (caller must have locked and verified not empty)
static utl_list_node_t *prv_unlink_front(utl_list_t *list)
{
    utl_list_node_t *node = list->head;
    list->head = node->next;

    if (list->head != NULL) {
        list->head->prev = NULL;
    } else {
        list->tail = NULL;
    }
    list->size--;
    return node;
}

/*===========================================================================
 * Lifecycle management
 *===========================================================================*/

utl_err_t utl_list_create(const utl_list_cfg_t *cfg, utl_list_t **list)
{
    /* Output pointer must not be NULL */
    if (prv_param_invalid(list)) {
        return UTL_ERR_NULL;
    }

    /* Use default config or user-provided config */
    const utl_list_cfg_t *effective_cfg = (cfg != NULL) ? cfg : &kDefaultCfg;

    /* Allocate list control block */
    utl_list_t *new_list = (utl_list_t *)malloc(sizeof(utl_list_t));
    if (new_list == NULL) {
        return UTL_ERR_MEM;
    }

    /* Initialize members */
    new_list->head        = NULL;
    new_list->tail        = NULL;
    new_list->size        = 0;
    new_list->max_size    = effective_cfg->max_size;
    new_list->thread_safe = effective_cfg->thread_safe;

    /* Initialize mutex as needed */
    if (new_list->thread_safe) {
        utl_err_t ret = os_mutex_create(&new_list->mutex);
        if (UTL_FAIL(ret)) {
            free(new_list);
            return UTL_ERR_LOCK;
        }
    }

    *list = new_list;
    return UTL_OK;
}

void utl_list_destroy(utl_list_t *list)
{
    if (list == NULL) {
        return;
    }

    /* Destroy all nodes */
    utl_list_node_t *current = list->head;
    while (current != NULL) {
        utl_list_node_t *next = current->next;
        prv_node_free(current);
        current = next;
    }

    /* Destroy mutex */
    if (list->thread_safe && list->mutex != NULL) {
        os_mutex_destroy(list->mutex);
        list->mutex = NULL;
    }

    /* Clear control block and free */
    memset(list, 0, sizeof(utl_list_t));
    free(list);
}

/*===========================================================================
 * Insertion and deletion operations
 *===========================================================================*/

utl_err_t utl_list_push_back(utl_list_t *list, void *data)
{
    if (prv_param_invalid(list) || prv_param_invalid(data)) {
        return UTL_ERR_NULL;
    }

    int lock_ret = prv_lock(list);
    if (lock_ret != 0) {
        return UTL_ERR_LOCK;
    }

    /* Capacity check */
    if (prv_is_full(list)) {
        prv_unlock(list);
        return UTL_ERR_FULL;
    }

    /* Allocate new node */
    utl_list_node_t *node = prv_node_alloc(data);
    if (node == NULL) {
        prv_unlock(list);
        return UTL_ERR_MEM;
    }

    prv_link_back(list, node);
    prv_unlock(list);
    return UTL_OK;
}

utl_err_t utl_list_push_front(utl_list_t *list, void *data)
{
    if (prv_param_invalid(list) || prv_param_invalid(data)) {
        return UTL_ERR_NULL;
    }

    int lock_ret = prv_lock(list);
    if (lock_ret != 0) {
        return UTL_ERR_LOCK;
    }

    if (prv_is_full(list)) {
        prv_unlock(list);
        return UTL_ERR_FULL;
    }

    utl_list_node_t *node = prv_node_alloc(data);
    if (node == NULL) {
        prv_unlock(list);
        return UTL_ERR_MEM;
    }

    prv_link_front(list, node);
    prv_unlock(list);
    return UTL_OK;
}

utl_err_t utl_list_pop_back(utl_list_t *list)
{
    if (prv_param_invalid(list)) {
        return UTL_ERR_NULL;
    }

    int lock_ret = prv_lock(list);
    if (lock_ret != 0) {
        return UTL_ERR_LOCK;
    }

    if (prv_is_empty(list)) {
        prv_unlock(list);
        return UTL_ERR_EMPTY;
    }

    utl_list_node_t *node = prv_unlink_back(list);
    prv_unlock(list);
    prv_node_free(node);
    return UTL_OK;
}

utl_err_t utl_list_pop_front(utl_list_t *list)
{
    if (prv_param_invalid(list)) {
        return UTL_ERR_NULL;
    }

    int lock_ret = prv_lock(list);
    if (lock_ret != 0) {
        return UTL_ERR_LOCK;
    }

    if (prv_is_empty(list)) {
        prv_unlock(list);
        return UTL_ERR_EMPTY;
    }

    utl_list_node_t *node = prv_unlink_front(list);
    prv_unlock(list);
    prv_node_free(node);
    return UTL_OK;
}

utl_err_t utl_list_remove(utl_list_t *list, void *data)
{
    if (prv_param_invalid(list) || prv_param_invalid(data)) {
        return UTL_ERR_NULL;
    }

    int lock_ret = prv_lock(list);
    if (lock_ret != 0) {
        return UTL_ERR_LOCK;
    }

    /* Traverse to find a matching node (pointer value comparison) */
    utl_list_node_t *current = list->head;
    while (current != NULL) {
        if (current->data == data) {
            /* Unlink node */
            if (current->prev != NULL) {
                current->prev->next = current->next;
            } else {
                list->head = current->next;
            }

            if (current->next != NULL) {
                current->next->prev = current->prev;
            } else {
                list->tail = current->prev;
            }

            list->size--;
            prv_unlock(list);
            prv_node_free(current);
            return UTL_OK;
        }
        current = current->next;
    }

    prv_unlock(list);
    return UTL_ERR_NOT_FOUND;
}

utl_err_t utl_list_clear(utl_list_t *list)
{
    if (prv_param_invalid(list)) {
        return UTL_ERR_NULL;
    }

    int lock_ret = prv_lock(list);
    if (lock_ret != 0) {
        return UTL_ERR_LOCK;
    }

    utl_list_node_t *current = list->head;
    while (current != NULL) {
        utl_list_node_t *next = current->next;
        prv_node_free(current);
        current = next;
    }

    list->head = NULL;
    list->tail = NULL;
    list->size = 0;

    prv_unlock(list);
    return UTL_OK;
}

/*===========================================================================
 * Query operations
 *===========================================================================*/

utl_err_t utl_list_front(const utl_list_t *list, void **data)
{
    if (prv_param_invalid(list) || prv_param_invalid(data)) {
        return UTL_ERR_NULL;
    }

    /* Note: the const_cast here is for locking; the lock operation does not modify the list, but mutex requires non-const */
    int lock_ret = prv_lock((utl_list_t *)list);
    if (lock_ret != 0) {
        return UTL_ERR_LOCK;
    }

    if (prv_is_empty(list)) {
        prv_unlock((utl_list_t *)list);
        return UTL_ERR_EMPTY;
    }

    *data = list->head->data;
    prv_unlock((utl_list_t *)list);
    return UTL_OK;
}

utl_err_t utl_list_back(const utl_list_t *list, void **data)
{
    if (prv_param_invalid(list) || prv_param_invalid(data)) {
        return UTL_ERR_NULL;
    }

    int lock_ret = prv_lock((utl_list_t *)list);
    if (lock_ret != 0) {
        return UTL_ERR_LOCK;
    }

    if (prv_is_empty(list)) {
        prv_unlock((utl_list_t *)list);
        return UTL_ERR_EMPTY;
    }

    *data = list->tail->data;
    prv_unlock((utl_list_t *)list);
    return UTL_OK;
}

utl_err_t utl_list_at(const utl_list_t *list, size_t index, void **data)
{
    if (prv_param_invalid(list) || prv_param_invalid(data)) {
        return UTL_ERR_NULL;
    }

    int lock_ret = prv_lock((utl_list_t *)list);
    if (lock_ret != 0) {
        return UTL_ERR_LOCK;
    }

    if (prv_is_empty(list)) {
        prv_unlock((utl_list_t *)list);
        return UTL_ERR_EMPTY;
    }

    if (index >= list->size) {
        prv_unlock((utl_list_t *)list);
        return UTL_ERR_NOT_FOUND;
    }

    /* Choose to traverse from head or tail based on index position, reducing average steps */
    utl_list_node_t *current = NULL;
    if (index <= list->size / 2) {
        /* Traverse from head */
        current = list->head;
        for (size_t i = 0; i < index; i++) {
            current = current->next;
        }
    } else {
        /* Traverse from tail */
        current = list->tail;
        for (size_t i = list->size - 1; i > index; i--) {
            current = current->prev;
        }
    }

    *data = current->data;
    prv_unlock((utl_list_t *)list);
    return UTL_OK;
}

utl_err_t utl_list_size(const utl_list_t *list, size_t *size)
{
    if (prv_param_invalid(list) || prv_param_invalid(size)) {
        return UTL_ERR_NULL;
    }

    int lock_ret = prv_lock((utl_list_t *)list);
    if (lock_ret != 0) {
        return UTL_ERR_LOCK;
    }

    *size = list->size;
    prv_unlock((utl_list_t *)list);
    return UTL_OK;
}

/*===========================================================================
 * In-place transformation
 *===========================================================================*/

utl_err_t utl_list_reverse(utl_list_t *list)
{
    if (prv_param_invalid(list)) {
        return UTL_ERR_NULL;
    }

    int lock_ret = prv_lock(list);
    if (lock_ret != 0) {
        return UTL_ERR_LOCK;
    }

    /* Empty or single-node list does not need reversal */
    if (list->size <= 1) {
        prv_unlock(list);
        return UTL_OK;
    }

    utl_list_node_t *current = list->head;
    utl_list_node_t *temp    = NULL;

    /* Save old head and tail */
    utl_list_node_t *old_head = list->head;

    while (current != NULL) {
        /* Swap prev/next pointers */
        temp = current->prev;
        current->prev = current->next;
        current->next = temp;

        /* Move to the original next (which is now prev) */
        current = current->prev;
    }

    /* At this point temp points to the predecessor of the original tail (predecessor of new head after reversal) */
    /* New head = original tail, new tail = original head */
    list->head = (temp != NULL) ? temp->prev : old_head;
    list->tail = old_head;

    prv_unlock(list);
    return UTL_OK;
}

/*===========================================================================
 * Snapshot mechanism
 *===========================================================================*/

utl_err_t utl_list_snapshot(const utl_list_t *list, utl_list_snapshot_t *snap)
{
    if (prv_param_invalid(list) || prv_param_invalid(snap)) {
        return UTL_ERR_NULL;
    }

    int lock_ret = prv_lock((utl_list_t *)list);
    if (lock_ret != 0) {
        return UTL_ERR_LOCK;
    }

    if (prv_is_empty(list)) {
        prv_unlock((utl_list_t *)list);
        snap->data  = NULL;
        snap->count = 0;
        return UTL_ERR_EMPTY;
    }

    /* Allocate data pointer array */
    void **data_array = (void **)malloc(sizeof(void *) * list->size);
    if (data_array == NULL) {
        prv_unlock((utl_list_t *)list);
        snap->data  = NULL;
        snap->count = 0;
        return UTL_ERR_MEM;
    }

    /* Traverse list, shallow copy all data pointers */
    utl_list_node_t *current = list->head;
    size_t idx = 0;
    while (current != NULL) {
        data_array[idx++] = current->data;
        current = current->next;
    }

    snap->data  = data_array;
    snap->count = list->size;

    prv_unlock((utl_list_t *)list);
    return UTL_OK;
}

void utl_list_snapshot_destroy(utl_list_snapshot_t *snap)
{
    if (snap == NULL) {
        return;
    }

    if (snap->data != NULL) {
        free(snap->data);
        snap->data = NULL;
    }
    snap->count = 0;
}
