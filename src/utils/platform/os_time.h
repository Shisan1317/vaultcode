/**
 * @file    os_time.h
 * @brief   Cross-platform time/timer abstraction interface — cross-project reusable
 * @author  RenJiaqi
 * @version 1.0.0
 * @date    2026-07-07
 *
 * @copyright Copyright (c) 2025-2026 VaultCode. All rights reserved.
 *
 * Description:
 *   - Provides millisecond-level tick counting and sleep interfaces
 *   - Tick is used for relative timing (performance measurement/timeout check), not wall clock time
 */

#ifndef OS_TIME_H
#define OS_TIME_H

#include "utl_types.h"
#include "utl_err.h"
#include "utl_config.h"

UTL_EXTERN_C_BEGIN

/*===========================================================================
 * Type definitions
 *===========================================================================*/

/// @brief Millisecond-level timestamp (tick) type
typedef uint64_t os_tick_t;

/*===========================================================================
 * API
 *===========================================================================*/

/**
 * @brief Get millisecond-level tick count since system boot
 * @return Current tick value (milliseconds)
 * @note  Used for relative time calculation, not guaranteed to align with wall clock time
 */
os_tick_t os_get_tick_ms(void);

/**
 * @brief Suspend the current thread for the specified number of milliseconds
 * @param[in] ms  Sleep duration in milliseconds, 0 means yield the CPU time slice
 */
void os_sleep_ms(uint32_t ms);

/**
 * @brief Get formatted time string (wall clock time)
 * @param[out] buf   Output buffer
 * @param[in]  size  Buffer size (recommended >= 32)
 * @return UTL_OK / UTL_ERR_NULL
 * @note  Format: "YYYY-MM-DD HH:MM:SS"
 */
utl_err_t os_get_time_str(char *buf, size_t size);

UTL_EXTERN_C_END

#endif /* OS_TIME_H */
