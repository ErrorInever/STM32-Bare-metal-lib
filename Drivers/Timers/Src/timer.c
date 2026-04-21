#include "timer.h"
#include "stm32f446xx.h"
#include <assert.h>
#include <stddef.h>
#include <stdint.h>

static timer_basic_callback_t timer_basic_callback[2] = {NULL, NULL};
static void *context_callback[2];

static uint32_t get_timer_basic_index(TIM_TypeDef *instance) {
    return (instance == TIM6) ? 0 : 1; // TIM6 0 idx, TIM7 1 idx
}

void timer_basic_set_callback(TIM_TypeDef *instance, timer_basic_callback_t callback, void *context) {
    uint32_t idx = get_timer_basic_index(instance);
    timer_basic_callback[idx] = callback;
    context_callback[idx] = context;
}

static void enable_clock(TIM_TypeDef *instance) {
    if(instance == TIM6) { RCC->APB1ENR |= RCC_APB1ENR_TIM6EN; (void)RCC->APB1ENR; }
    else if(instance == TIM7) { RCC->APB1ENR |= RCC_APB1ENR_TIM7EN; (void)RCC->APB1ENR; }
}

void timer_basic_init_ms(timer_basic_t *timer, uint32_t period_ms) {
    // RCC
    assert(timer->instance != NULL);
    assert(timer->bus_freq >= 1U && timer->bus_freq <= 184U);
    enable_clock(timer->instance);
    // Configure PSC and ARR

    timer->instance->PSC = (timer->bus_freq * 100) - 1; // PSC = 10Khz. CNT increment each 0.1ms
    timer->instance->ARR = (period_ms * 10) - 1; // For 1000 ms: 10000 - 1 = 9999
    // Forced update
    timer->instance->EGR |= TIM_EGR_UG;
    timer->instance->SR &= ~TIM_SR_UIF; // reset flag
    // Enable interrupt
    timer->instance->DIER |= TIM_DIER_UIE;
    // Enable interrupt in NVIC
    if(timer->instance == TIM6) NVIC_EnableIRQ(TIM6_DAC_IRQn);
    else if(timer->instance == TIM7) NVIC_EnableIRQ(TIM7_IRQn);
}

void timer_basic_delay_ms(const timer_basic_t *timer, uint32_t ms) {
    bool was_disabled = !(timer->instance->CR1 & TIM_CR1_CEN);
    if (was_disabled) timer->instance->CR1 |= TIM_CR1_CEN;

    uint32_t start_tick = timer->instance->CNT;
    uint32_t wait_ticks = ms * 10;

    while ((uint16_t)(timer->instance->CNT - start_tick) < wait_ticks);
    if (was_disabled) timer->instance->CR1 &= ~TIM_CR1_CEN;
}

void TIM6_DAC_IRQHandler(void) {
    if(TIM6->SR & TIM_SR_UIF) {
        TIM6->SR &= ~TIM_SR_UIF; // reset flag in status register
        if(timer_basic_callback[0]) {
            timer_basic_callback[0](context_callback[0]);
        }
    }
}

void TIM7_IRQHandler(void) {
    if(TIM7->SR & TIM_SR_UIF) {
        TIM7->SR &= ~TIM_SR_UIF; // reset flag in status register
        if(timer_basic_callback[1]) {
            timer_basic_callback[1](context_callback[1]);
        }
    }
}