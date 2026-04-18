#include "timer.h"
#include "stm32f446xx.h"
#include <assert.h>
#include <stddef.h>
#include <stdint.h>


static void enable_clock(TIM_TypeDef instance) {
    if(instance == TIM6) { RCC->APB1ENR |= RCC_APB1ENR_TIM6EN; (void)RCC->APB1ENR; }
    else if(instance == TIM7) { RCC->APB1ENR |= RCC_APB1ENR_TIM7EN; (void)RCC->APB1ENR; }
}


void timer_basic_init_ms(timer_basic_t timer, uint32_t period_ms) {
    // RCC
    assert(timer.instance != NULL);
    assert(timer.bus_freq > 1U && timer.bus_freq < 184U);
    enable_clock(timer.instance);
    // Configure PSC and ARR

    timer.instance->PSC = (timer.bus_freq * 100) - 1; // PSC = 10Khz. CNT increment each 0.1ms
    timer.instance->ARR = (period_ms * 10) - 1; // For 1000 ms: 10000 - 1 = 9999
    // Forced update
    timer.instance->EGR |= TIM_EGR_UG;
    timer.instance->SR &= ~TIM_SR_UIF; // reset flag
    // Enable interrupt
    timer.instance->DIER |= TIM_DIER_UIE;
    // Enable timer
    timer.instance->CR1 |= TIM_CR1_CEN;
    // Enable interrupt in NVIC
    if(timer.instance == TIM6) NVIC_EnableIRQ(TIM6_DAC_IRQn);
    else if(timer.instance == TIM7) NVIC_EnableIRQ(TIM7_IRQn);
}