#ifndef SYSTICK_H_
#define SYSTICK_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32f446xx.h"
#include <stdint.h>
#include <stdbool.h>

volatile uint32_t ms_ticks; // ms

// Init
// 
// Interrupt each ms
void systick_config_ms(uint32_t cpu_mhz);
// Operations
//
// Delay ms
void delay_ms(uint32_t ms);
// Systick is enable?
static inline bool systick_is_enable(void) {
    return (SysTick->CTRL & SysTick_CTRL_ENABLE_Msk) ? 1 : 0;
}
// Get current value of CNT
static inline uint32_t systick_get_val(void) {
    return (SysTick->VAL);
}
// Reset CNT, reset COUNTFLAG in CTRL
static inline void systick_reset(void) {
    SysTick->VAL = 0UL;
}

#ifdef __cplusplus
}
#endif

#endif
