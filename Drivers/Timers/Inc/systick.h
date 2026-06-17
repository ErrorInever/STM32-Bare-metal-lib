/**
 * @file systick.h
 * @brief Core ARM Cortex-M SysTick Timer Peripheral Driver.
 * * Provides an interface to configure the system tick timer for precise 
 * millisecond timekeeping, generation of periodic system exceptions, 
 * and blocking delay functionalities.
 * * @author ErrorInever
 * @date 2026-06-17
 */

#ifndef SYSTICK_H_
#define SYSTICK_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32f446xx.h"
#include <stdint.h>
#include <stdbool.h>


/* ========================================================================== */
/* Initialization                                                             */
/* ========================================================================== */

/**
 * @brief Initializes and enables the SysTick timer to trigger an interrupt every 1 ms.
 * * Configures the reload register (LOAD) based on the current CPU frequency,
 * sets the processor clock source, enables the SysTick exception request, 
 * and starts the counter.
 * * @pre the computed ticks value must fit within the 24-bit SysTick LOAD register space.
 * @param[in] cpu_mhz The current core processor clock frequency expressed in MHz (e.g., 84 or 180).
 */
void systick_config_ms(uint32_t cpu_mhz);


/* ========================================================================== */
/* Operations                                                                 */
/* ========================================================================== */

/**
 * @brief Executes a blocking, synchronous millisecond delay.
 * * Polls the volatile system tick counter variable until the specified 
 * duration has elapsed. Safe against 32-bit overflow roll-overs due to unsigned delta math.
 * * @note Requires @ref systick_config_ms to have been called first, and global interrupts enabled.
 * @param[in] ms Time duration to block the execution thread in milliseconds.
 */
void delay_ms(uint32_t ms);


/**
 * @brief Checks whether the SysTick peripheral counter is currently active.
 * * Queries the hardware state of the `ENABLE` bit within the SysTick Control and Status Register.
 * * @return true if the SysTick counter is enabled, false if it is disabled.
 */
static inline bool systick_is_enable(void) {
    return (SysTick->CTRL & SysTick_CTRL_ENABLE_Msk) ? 1 : 0;
}


/**
 * @brief Retrieves the current value of the internal SysTick counter.
 * * Reads the 24-bit `VAL` register representing the remaining ticks 
 * until the next down-count underflow.
 * * @return Current 24-bit down-counter tick value.
 */
static inline uint32_t systick_get_val(void) {
    return (SysTick->VAL);
}


/**
 * @brief Resets the current counter register and clears the COUNTFLAG status.
 * * Writing any arbitrary value to the `VAL` register clears both the counter 
 * value to 0 and clears the hardware `COUNTFLAG` indicator bit in the `CTRL` register.
 */
static inline void systick_reset(void) {
    SysTick->VAL = 0UL;
}

#ifdef __cplusplus
}
#endif

#endif /* SYSTICK_H_ */
