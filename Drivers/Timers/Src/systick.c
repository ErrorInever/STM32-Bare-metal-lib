#include "systick.h"
#include <stdint.h>

volatile uint32_t ms_ticks = 0;

void systick_config_ms(uint32_t cpu_mhz) {
    uint32_t ticks = (cpu_mhz * 1000U) - 1UL; // for example, 84 Mhz = 84 * 1000 = 84000 - 1.
    assert(ticks <= 0xFFFFFFUL); // too much delay
    SysTick->LOAD = ticks;
    SysTick->VAL = 0UL;
    SysTick->CTRL = SysTick_CTRL_CLKSOURCE_Msk | 
                     SysTick_CTRL_TICKINT_Msk  | 
                     SysTick_CTRL_ENABLE_Msk;
}

void SysTick_Handler(void) {
    ms_ticks++;
}

void delay_ms(uint32_t ms) {
    uint32_t start = ms_ticks;
    while((ms_ticks - start) < ms);
}