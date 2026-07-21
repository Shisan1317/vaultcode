/**
 * @file    os_atomic.h
 * @brief   Cross-platform atomic operations — cross-project reusable (header-only, compiler builtins)
 * @author  RenJiaqi
 * @version 1.0.0
 * @date    2026-07-07
 *
 * @copyright Copyright (c) 2025-2026 VaultCode. All rights reserved.
 *
 * Description:
 *   - Based on GCC/Clang __atomic builtins or MSVC Interlocked API
 *   - Header-only implementation, zero .o dependency
 *   - Used for lightweight reference counting, lock-free counters, etc.
 */

#ifndef OS_ATOMIC_H
#define OS_ATOMIC_H

#include "utl_types.h"
#include "utl_config.h"

/*===========================================================================
 * 32-bit atomic operations
 *===========================================================================*/

#if defined(__GNUC__) || defined(__clang__)

/* ---- GCC / Clang: __atomic builtins ---- */

/**
 * @brief Atomic add (32-bit)
 * @return Old value before the operation
 */
UTL_INLINE int32_t os_atomic_add32(volatile int32_t *ptr, int32_t val)
{
    return __atomic_fetch_add(ptr, val, __ATOMIC_SEQ_CST);
}

UTL_INLINE int32_t os_atomic_sub32(volatile int32_t *ptr, int32_t val)
{
    return __atomic_fetch_sub(ptr, val, __ATOMIC_SEQ_CST);
}

UTL_INLINE int32_t os_atomic_inc32(volatile int32_t *ptr)
{
    return __atomic_fetch_add(ptr, 1, __ATOMIC_SEQ_CST);
}

UTL_INLINE int32_t os_atomic_dec32(volatile int32_t *ptr)
{
    return __atomic_fetch_sub(ptr, 1, __ATOMIC_SEQ_CST);
}

UTL_INLINE int32_t os_atomic_load32(const volatile int32_t *ptr)
{
    return __atomic_load_n(ptr, __ATOMIC_SEQ_CST);
}

UTL_INLINE void os_atomic_store32(volatile int32_t *ptr, int32_t val)
{
    __atomic_store_n(ptr, val, __ATOMIC_SEQ_CST);
}

/**
 * @brief CAS: if *ptr == expected, set to desired and return true
 */
UTL_INLINE utl_bool_t os_atomic_cas32(volatile int32_t *ptr,
                                       int32_t expected, int32_t desired)
{
    return __atomic_compare_exchange_n(ptr, &expected, desired,
                                       0, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
}

#elif defined(_MSC_VER)

/* ---- MSVC: Interlocked API ---- */
#include <windows.h>

UTL_INLINE int32_t os_atomic_add32(volatile int32_t *ptr, int32_t val)
{
    return InterlockedExchangeAdd((LONG volatile *)ptr, val);
}

UTL_INLINE int32_t os_atomic_sub32(volatile int32_t *ptr, int32_t val)
{
    return InterlockedExchangeAdd((LONG volatile *)ptr, -val);
}

UTL_INLINE int32_t os_atomic_inc32(volatile int32_t *ptr)
{
    return InterlockedIncrement((LONG volatile *)ptr) - 1;
}

UTL_INLINE int32_t os_atomic_dec32(volatile int32_t *ptr)
{
    return InterlockedDecrement((LONG volatile *)ptr) + 1;
}

UTL_INLINE int32_t os_atomic_load32(const volatile int32_t *ptr)
{
    return InterlockedCompareExchange((LONG volatile *)ptr, 0, 0);
}

UTL_INLINE void os_atomic_store32(volatile int32_t *ptr, int32_t val)
{
    InterlockedExchange((LONG volatile *)ptr, val);
}

UTL_INLINE utl_bool_t os_atomic_cas32(volatile int32_t *ptr,
                                       int32_t expected, int32_t desired)
{
    return (InterlockedCompareExchange((LONG volatile *)ptr, desired, expected)
            == expected);
}

#else
    #error "Unsupported compiler: no atomic implementation available"
#endif

/*===========================================================================
 * 64-bit atomic operations (optional, only when supported by platform)
 *===========================================================================*/

#if defined(__GNUC__) || defined(__clang__)

UTL_INLINE int64_t os_atomic_add64(volatile int64_t *ptr, int64_t val)
{
    return __atomic_fetch_add(ptr, val, __ATOMIC_SEQ_CST);
}

UTL_INLINE int64_t os_atomic_load64(const volatile int64_t *ptr)
{
    return __atomic_load_n(ptr, __ATOMIC_SEQ_CST);
}

UTL_INLINE utl_bool_t os_atomic_cas64(volatile int64_t *ptr,
                                       int64_t expected, int64_t desired)
{
    return __atomic_compare_exchange_n(ptr, &expected, desired,
                                       0, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
}

#endif /* GNUC || Clang */

#endif /* OS_ATOMIC_H */
