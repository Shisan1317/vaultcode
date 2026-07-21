/**
 * @file    utl_types.h
 * @brief   Unified base type definitions — cross-platform, cross-project reusable
 * @author  RenJiaqi
 * @version 1.0.0
 * @date    2026-07-07
 *
 * @copyright Copyright (c) 2025-2026 VaultCode. All rights reserved.
 *
 * Description:
 *   - Provides unified base type aliases to avoid direct use of C standard types
 *   - When porting to non-standard platforms, only this file needs modification
 *   - This header is zero-dependency and can be included by any module
 */

#ifndef UTL_TYPES_H
#define UTL_TYPES_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

/*===========================================================================
 * Boolean type
 *===========================================================================*/

typedef bool utl_bool_t;

#define UTL_TRUE   true
#define UTL_FALSE  false

/*===========================================================================
 * Byte & character types
 *===========================================================================*/

typedef uint8_t   utl_byte_t;
typedef char      utl_char_t;

/*===========================================================================
 * Fixed-width integer type aliases
 *===========================================================================*/

typedef int8_t    utl_i8_t;
typedef int16_t   utl_i16_t;
typedef int32_t   utl_i32_t;
typedef int64_t   utl_i64_t;

typedef uint8_t   utl_u8_t;
typedef uint16_t  utl_u16_t;
typedef uint32_t  utl_u32_t;
typedef uint64_t  utl_u64_t;

/*===========================================================================
 * Floating-point type aliases
 *===========================================================================*/

typedef float     utl_f32_t;
typedef double    utl_f64_t;

/*===========================================================================
 * General pointer & size types
 *===========================================================================*/

typedef size_t    utl_size_t;
typedef ptrdiff_t utl_ptrdiff_t;

/*===========================================================================
 * Null pointer constant (C89 compatible)
 *===========================================================================*/

#ifndef UTL_NULL
#define UTL_NULL  NULL
#endif

/*===========================================================================
 * Compiler helper macros
 *===========================================================================*/

/// @brief Inline function declaration (C99 / GNU89 compatible)
#if defined(__GNUC__) || defined(__clang__)
    #define UTL_INLINE static inline __attribute__((always_inline))
#elif defined(_MSC_VER)
    #define UTL_INLINE static __forceinline
#else
    #define UTL_INLINE static inline
#endif

/// @brief Mark unused variables/parameters to suppress compiler warnings
#if defined(__GNUC__) || defined(__clang__)
    #define UTL_UNUSED(x)  (void)(x)
#else
    #define UTL_UNUSED(x)  (x)
#endif

/// @brief Compile-time array length
#define UTL_ARRAY_SIZE(arr)  (sizeof(arr) / sizeof((arr)[0]))

/// @brief Offset macro
#ifndef utl_offsetof
#define utl_offsetof(type, member)  offsetof(type, member)
#endif

/// @brief Get containing struct pointer from member pointer
#ifndef utl_container_of
#define utl_container_of(ptr, type, member) \
    ((type *)((char *)(ptr) - utl_offsetof(type, member)))
#endif

#endif /* UTL_TYPES_H */
