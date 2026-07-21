/**
 * @file    utl_config.h
 * @brief   Compiler/platform/architecture detection & cross-platform compatibility macros — cross-project reusable
 * @author  RenJiaqi
 * @version 1.0.0
 * @date    2026-07-07
 *
 * @copyright Copyright (c) 2025-2026 VaultCode. All rights reserved.
 *
 * Description:
 *   - Provides compile-time platform detection to drive conditional compilation in each module
 *   - Provides C/C++ mixed compilation protection macros (EXTERN_C)
 *   - This header is zero-dependency and should be the first common header included by every module
 *
 * Platform porting guide:
 *   When adding a new platform, add OS/Compiler/Arch detection branches in the
 *   corresponding sections of this file. No changes needed in upper-layer code.
 */

#ifndef UTL_CONFIG_H
#define UTL_CONFIG_H

/*===========================================================================
 * Operating system detection
 *===========================================================================*/

#if defined(__ANDROID__)
    #define UTL_OS_ANDROID  1
    #define UTL_OS_LINUX    1   /* Android is Linux-based */
    #define UTL_OS_NAME     "Android"
#elif defined(__linux__)
    #define UTL_OS_LINUX    1
    #define UTL_OS_NAME     "Linux"
#elif defined(_WIN32) || defined(_WIN64)
    #define UTL_OS_WINDOWS  1
    #define UTL_OS_NAME     "Windows"
#elif defined(__APPLE__) && defined(__MACH__)
    #define UTL_OS_APPLE    1
    #if TARGET_OS_IPHONE
        #define UTL_OS_IOS  1
        #define UTL_OS_NAME "iOS"
    #else
        #define UTL_OS_MACOS 1
        #define UTL_OS_NAME "macOS"
    #endif
#else
    #warning "Unknown operating system — some platform features may not work"
    #define UTL_OS_UNKNOWN  1
    #define UTL_OS_NAME     "Unknown"
#endif

/*===========================================================================
 * Compiler detection
 *===========================================================================*/

#if defined(__GNUC__)
    #define UTL_CC_GCC      1
    #define UTL_CC_NAME     "GCC"
    #if defined(__clang__)
        #undef  UTL_CC_NAME
        #define UTL_CC_NAME "Clang"
    #endif
#elif defined(_MSC_VER)
    #define UTL_CC_MSVC     1
    #define UTL_CC_NAME     "MSVC"
#else
    #define UTL_CC_UNKNOWN  1
    #define UTL_CC_NAME     "Unknown"
#endif

/*===========================================================================
 * Target architecture detection
 *===========================================================================*/

#if defined(__x86_64__) || defined(_M_X64)
    #define UTL_ARCH_X86_64   1
    #define UTL_ARCH_BITS     64
#elif defined(__i386__) || defined(_M_IX86)
    #define UTL_ARCH_X86      1
    #define UTL_ARCH_BITS     32
#elif defined(__aarch64__)
    #define UTL_ARCH_ARM64    1
    #define UTL_ARCH_BITS     64
#elif defined(__arm__)
    #define UTL_ARCH_ARM      1
    #define UTL_ARCH_BITS     32
#else
    #define UTL_ARCH_UNKNOWN  1
    #define UTL_ARCH_BITS     0
#endif

/*===========================================================================
 * C / C++ mixed compilation protection
 *===========================================================================*/

#ifdef __cplusplus
    #define UTL_EXTERN_C_BEGIN  extern "C" {
    #define UTL_EXTERN_C_END    }
#else
    #define UTL_EXTERN_C_BEGIN
    #define UTL_EXTERN_C_END
#endif

/*===========================================================================
 * Compiler attributes & branch prediction
 *===========================================================================*/

/// @brief Branch prediction hint: condition is likely true
#if defined(__GNUC__) || defined(__clang__)
    #define UTL_LIKELY(x)     __builtin_expect(!!(x), 1)
    #define UTL_UNLIKELY(x)   __builtin_expect(!!(x), 0)
#else
    #define UTL_LIKELY(x)     (x)
    #define UTL_UNLIKELY(x)   (x)
#endif

/// @brief Mark function as deprecated
#if defined(__GNUC__) || defined(__clang__)
    #define UTL_DEPRECATED(msg)  __attribute__((deprecated(msg)))
#elif defined(_MSC_VER)
    #define UTL_DEPRECATED(msg)  __declspec(deprecated(msg))
#else
    #define UTL_DEPRECATED(msg)
#endif

/// @brief Mark function as printf-style (for compile-time format checking)
#if defined(__GNUC__) || defined(__clang__)
    #define UTL_PRINTF_LIKE(fmt_idx, arg_idx) \
        __attribute__((format(printf, fmt_idx, arg_idx)))
#else
    #define UTL_PRINTF_LIKE(fmt_idx, arg_idx)
#endif

#endif /* UTL_CONFIG_H */
