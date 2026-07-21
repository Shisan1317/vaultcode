/**
 * @file    utl_list.h
 * @brief   General-purpose doubly-linked list module — object-oriented encapsulation interface
 * @author  RenJiaqi
 * @version 2.0.0
 * @date    2026-07-07
 *
 * @copyright Copyright (c) 2025-2026 VaultCode. All rights reserved.
 *
 * Changelog:
 *   v1.0.0  2025-06-24  RenJiaqi  Initial version: basic doubly-linked list CRUD
 *   v2.0.0  2026-07-07  RenJiaqi  OOP refactoring:
 *                                      - List struct forward-declared, internal members fully private
 *                                      - Unified error code returns, distinguishing various error scenarios
 *                                      - Supports thread_safe / max_size configurable at creation
 *                                      - Added index-based lookup at / total node count size
 *                                      - Novel snapshot mechanism: copy all data pointers before iteration,
 *                                        offline iteration without blocking writes, greatly improving multi-threaded concurrency
 *                                      - Companion snapshot destroy interface to prevent memory leaks
 *                                      - Compatible with C++ mixed compilation, follows engineering coding standards
 *
 * Usage example:
 * @code
 *   utl_list_cfg_t cfg = { .thread_safe = true, .max_size = 1024 };
 *   utl_list_t *list = NULL;
 *   utl_list_err_t ret = utl_list_create(&cfg, &list);
 *   if (ret != UTL_LIST_OK) { log error; return; }
 *
 *   ret = utl_list_push_back(list, my_data);
 *   size_t count = 0;
 *   utl_list_size(list, &count);
 *
 *   // Snapshot iteration (does not block write operations)
 *   utl_list_snapshot_t snap;
 *   if (utl_list_snapshot(list, &snap) == UTL_LIST_OK) {
 *       for (size_t i = 0; i < snap.count; i++) {
 *           process(snap.data[i]);
 *       }
 *       utl_list_snapshot_destroy(&snap);
 *   }
 *
 *   utl_list_destroy(list);
 * @endcode
 */

#ifndef UTL_LIST_H
#define UTL_LIST_H

#include <stddef.h>
#include "utl_types.h"
#include "utl_err.h"
#include "utl_config.h"

UTL_EXTERN_C_BEGIN

/*===========================================================================
 * Type forward declarations (opaque pointers, external code only holds handles and cannot directly tamper with internal members)
 *===========================================================================*/

/// @brief List handle — internal struct definition in .c file, fully hidden from outside
typedef struct utl_list_obj utl_list_t;

/// @brief List snapshot handle — offline data copy held during iteration
typedef struct utl_list_snapshot utl_list_snapshot_t;

/*===========================================================================
 * Error codes — uses project-global utl_err_t, consistent across all modules
 *===========================================================================*/

/// @brief Backward-compatible alias (recommend using utl_err_t directly)
typedef utl_err_t utl_list_err_t;

/* Backward-compatible constant aliases (recommend using UTL_OK / UTL_ERR_*) */
#define UTL_LIST_OK             UTL_OK
#define UTL_LIST_ERR_NULL       UTL_ERR_NULL
#define UTL_LIST_ERR_MEM        UTL_ERR_MEM
#define UTL_LIST_ERR_FULL       UTL_ERR_FULL
#define UTL_LIST_ERR_EMPTY      UTL_ERR_EMPTY
#define UTL_LIST_ERR_NOT_FOUND  UTL_ERR_NOT_FOUND
#define UTL_LIST_ERR_LOCK       UTL_ERR_LOCK

/*===========================================================================
 * Creation configuration struct
 *===========================================================================*/

/// @brief Configuration parameters for list creation
typedef struct {
    bool     thread_safe;   ///< true = enable internal mutex, false = non-thread-safe (higher single-threaded performance)
    uint32_t max_size;      ///< Maximum node count limit, 0 means unlimited
} utl_list_cfg_t;

/*===========================================================================
 * Snapshot struct (externally visible, used for offline iteration)
 *===========================================================================*/

struct utl_list_snapshot {
    void   **data;          ///< Data pointer array (shallow copy, freed uniformly by snapshot destroy interface)
    size_t   count;         ///< Total node count at snapshot time
};

/*===========================================================================
 * Lifecycle management
 *===========================================================================*/

/**
 * @brief Dynamically create a list instance (heap-allocated)
 * @param[in]  cfg   Configuration pointer, NULL means use default (non-thread-safe, unlimited capacity)
 * @param[out] list  Output list handle pointer
 * @return UTL_LIST_OK on success, UTL_LIST_ERR_NULL invalid argument, UTL_LIST_ERR_MEM out of memory
 */
utl_err_t utl_list_create(const utl_list_cfg_t *cfg, utl_list_t **list);

/**
 * @brief Destroy list instance, freeing all nodes and the list's own memory
 * @param[in] list  List handle (passing NULL is safely ignored)
 * @note  After destruction the handle becomes a dangling pointer; caller should set to NULL
 */
void utl_list_destroy(utl_list_t *list);

/*===========================================================================
 * Insertion and deletion operations (unified error code return)
 *===========================================================================*/

/**
 * @brief Push back — append an element at the end of the list
 * @param[in] list  List handle
 * @param[in] data  Data pointer (must not be NULL)
 * @return UTL_LIST_OK / UTL_LIST_ERR_NULL / UTL_LIST_ERR_MEM / UTL_LIST_ERR_FULL
 */
utl_err_t utl_list_push_back(utl_list_t *list, void *data);

/**
 * @brief Push front — insert an element at the head of the list
 * @param[in] list  List handle
 * @param[in] data  Data pointer (must not be NULL)
 * @return UTL_LIST_OK / UTL_LIST_ERR_NULL / UTL_LIST_ERR_MEM / UTL_LIST_ERR_FULL
 */
utl_err_t utl_list_push_front(utl_list_t *list, void *data);

/**
 * @brief Pop back — remove the element at the end of the list
 * @param[in] list  List handle
 * @return UTL_LIST_OK / UTL_LIST_ERR_NULL / UTL_LIST_ERR_EMPTY
 */
utl_err_t utl_list_pop_back(utl_list_t *list);

/**
 * @brief Pop front — remove the element at the head of the list
 * @param[in] list  List handle
 * @return UTL_LIST_OK / UTL_LIST_ERR_NULL / UTL_LIST_ERR_EMPTY
 */
utl_err_t utl_list_pop_front(utl_list_t *list);

/**
 * @brief Remove by value — remove the first node whose data pointer equals the given value
 * @param[in] list  List handle
 * @param[in] data  Data pointer to remove (compared by pointer value, not content)
 * @return UTL_LIST_OK / UTL_LIST_ERR_NULL / UTL_LIST_ERR_NOT_FOUND
 * @note  Only frees node memory, does NOT free the user data pointed to by data
 */
utl_err_t utl_list_remove(utl_list_t *list, void *data);

/**
 * @brief Clear list — remove all nodes
 * @param[in] list  List handle
 * @return UTL_LIST_OK / UTL_LIST_ERR_NULL
 * @note  Only frees node memory, does NOT free the user data pointed to by data
 */
utl_err_t utl_list_clear(utl_list_t *list);

/*===========================================================================
 * Query operations
 *===========================================================================*/

/**
 * @brief Get the element at the head of the list (does not remove)
 * @param[in]  list  List handle
 * @param[out] data  Output data pointer
 * @return UTL_LIST_OK / UTL_LIST_ERR_NULL / UTL_LIST_ERR_EMPTY
 */
utl_err_t utl_list_front(const utl_list_t *list, void **data);

/**
 * @brief Get the element at the tail of the list (does not remove)
 * @param[in]  list  List handle
 * @param[out] data  Output data pointer
 * @return UTL_LIST_OK / UTL_LIST_ERR_NULL / UTL_LIST_ERR_EMPTY
 */
utl_err_t utl_list_back(const utl_list_t *list, void **data);

/**
 * @brief Index-based lookup — get the data of the node at position index (0-based)
 * @param[in]  list   List handle
 * @param[in]  index  Index position, 0 means the first node
 * @param[out] data   Output data pointer
 * @return UTL_LIST_OK / UTL_LIST_ERR_NULL / UTL_LIST_ERR_EMPTY / UTL_LIST_ERR_NOT_FOUND
 * @note  O(n) time complexity; prefer arrays for frequent random access
 */
utl_err_t utl_list_at(const utl_list_t *list, size_t index, void **data);

/**
 * @brief Get the current total node count of the list
 * @param[in]  list  List handle
 * @param[out] size  Output node count
 * @return UTL_LIST_OK / UTL_LIST_ERR_NULL
 */
utl_err_t utl_list_size(const utl_list_t *list, size_t *size);

/*===========================================================================
 * In-place transformation
 *===========================================================================*/

/**
 * @brief Reverse the list in-place
 * @param[in] list  List handle
 * @return UTL_LIST_OK / UTL_LIST_ERR_NULL
 */
utl_err_t utl_list_reverse(utl_list_t *list);

/*===========================================================================
 * Snapshot mechanism — a powerful tool for multi-threaded concurrent iteration
 * Principle: Before iteration, shallow-copy all data pointers to an independent array.
 *            Subsequent offline iteration of the array does not hold the list lock,
 *            allowing the list to continue handling write operations normally.
 *            This greatly reduces lock contention and improves multi-threaded throughput.
 *===========================================================================*/

/**
 * @brief Create a list snapshot — copy all current data pointers to an independent array
 * @param[in]  list  List handle
 * @param[out] snap  Output snapshot object
 * @return UTL_LIST_OK / UTL_LIST_ERR_NULL / UTL_LIST_ERR_EMPTY / UTL_LIST_ERR_MEM / UTL_LIST_ERR_LOCK
 * @note  snap.data must be freed by the caller via utl_list_snapshot_destroy()
 * @note  The snapshot is a shallow copy; data[i] still points to the original user data. Do not free user data during iteration.
 * @note  In thread-safe mode, snapshot creation is lock-protected; in non-thread-safe mode, no lock overhead.
 */
utl_err_t utl_list_snapshot(const utl_list_t *list, utl_list_snapshot_t *snap);

/**
 * @brief Destroy a list snapshot — free the snapshot's internal array memory
 * @param[in] snap  Snapshot object pointer
 * @note  Only frees the snapshot array, does NOT free user data pointed to by data[i]
 * @note  Destroying the same snapshot more than once causes undefined behavior
 */
void utl_list_snapshot_destroy(utl_list_snapshot_t *snap);

UTL_EXTERN_C_END

#endif /* UTL_LIST_H */
