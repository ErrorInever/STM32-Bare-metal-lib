#include "timer.h"
#include "stm32f446xx.h"
#include <assert.h>
#include <stddef.h>
#include <stdint.h>

static timer_basic_callback_t timer_basic_callback[2] = {NULL, NULL};
static void *context_callback[2];

static uint32_t get_timer_basic_index(const timer_basic_t *timer) {
    return (timer->instance == TIM6) ? 0 : 1; // TIM6 0 idx, TIM7 1 idx
}

void timer_basic_set_callback(const timer_basic_t *timer, timer_basic_callback_t callback, void *context) {
    uint32_t idx = get_timer_basic_index(timer);
    timer_basic_callback[idx] = callback;
    context_callback[idx] = context;
}

static void enable_clock(TIM_TypeDef *instance) {
    if(instance == TIM6) { RCC->APB1ENR |= RCC_APB1ENR_TIM6EN }
    else if(instance == TIM7) { RCC->APB1ENR |= RCC_APB1ENR_TIM7EN }
    else if(instance == TIM2) { RCC->APB1ENR |= RCC_APB1ENR_TIM2EN }
    else if(instance == TIM3) { RCC->APB1ENR |= RCC_APB1ENR_TIM3EN }
    (void)RCC->APB1ENR; 
}

void timer_basic_init_ms(const timer_basic_t *timer, uint32_t period_ms) {
    // RCC
    assert(timer->instance != NULL);
    assert(timer->bus_freq >= 1U && timer->bus_freq <= 184U);
    enable_clock(timer->instance);
    // Configure PSC and ARR
    timer->instance->PSC = (timer->bus_freq * 100U) - 1U; // PSC = 10Khz. CNT increment each 0.1ms
    timer->instance->ARR = (period_ms * 10U) - 1U; // For 1000 ms: 10000 - 1 = 9999
    // Forced update
    timer->instance->EGR |= TIM_EGR_UG;
    timer->instance->SR &= ~TIM_SR_UIF; // reset flag
    // Enable interrupt
    timer->instance->DIER |= TIM_DIER_UIE;
    // Enable interrupt in NVIC
    if(timer->instance == TIM6) NVIC_EnableIRQ(TIM6_DAC_IRQn);
    else if(timer->instance == TIM7) NVIC_EnableIRQ(TIM7_IRQn);
}

void timer_general_init_ms(const timer_general_t *timer, uint32_t pwm_freq, uint32_t dc_percent) {
    assert(timer->instance != NULL);
    assert(timer->bus_freq >= 1U && timer->bus_freq <= 184U);
    enable_clock(timer->instance);
    // Configure PWM
    timer->instance->PSC = (timer->bus_freq * 100U) - 1U; // 10Khz 
    uint32_t period = 10000U / pwm_freq_hz;
    timer->instance->ARR = period - 1U;
    timer->instance->CCR1 = (period * dc_percent) / 100U;
}

void timer_general_pwm_channel_config(const timer_general_t *timer, uint8_t channel) {
    assert(timer->instance != NULL);
    assert(channel >= 1 && channel <= 4);

    switch (channel) {
        case 1:
            // PWM Mode 1 + preload enable for channel 1
            timer->instance->CCMR1 &= ~TIM_CCMR1_OC1M;
            timer->instance->CCMR1 |= (6U << TIM_CCMR1_OC1M_Pos) | TIM_CCMR1_OC1PE;
            // enable output channel 1
            timer->instance->CCER |= TIM_CCER_CC1E;
            break;
        
        case 2:
            // PWM Mode 1 + preload enable for channel 2
            timer->instance->CCMR1 &= ~TIM_CCMR1_OC2M;
            timer->instance->CCMR1 |= (6U << TIM_CCMR1_OC2M_Pos) | TIM_CCMR1_OC2PE;
            // enable output channel 2
            timer->instance->CCER |= TIM_CCER_CC2E;

        case 3:
            // PWM Mode 1 + preload enable for channel 3
            timer->instance->CCMR2 &= ~TIM_CCMR2_OC3M;
            timer->instance->CCMR2 |= (6U << TIM_CCMR2_OC3M_Pos) | TIM_CCMR2_OC3PE;
            // enable output channel 3
            timer->instance->CCER |= TIM_CCER_CC3E;
            break;

        case 4:
            // PWM Mode 1 + preload enable for channel 4
            timer->instance->CCMR2 &= ~TIM_CCMR2_OC4M;
            timer->instance->CCMR2 |= (6U << TIM_CCMR2_OC4M_Pos) | TIM_CCMR2_OC4PE;
            // enable output channel 4
            timer->instance->CCER |= TIM_CCER_CC4E;
            break;
    }
    // If this TIM1 or TIM8, need enable main output
    if (timer->instance == TIM1 || timer->instance == TIM8) {
        timer->instance->BDTR |= TIM_BDTR_MOE;
    }
}



void timer_basic_delay_ms(const timer_basic_t *timer, uint32_t ms) {
    bool was_disabled = !(timer->instance->CR1 & TIM_CR1_CEN);
    if (was_disabled) timer->instance->CR1 |= TIM_CR1_CEN;

    uint32_t start_tick = timer->instance->CNT;
    uint32_t wait_ticks = ms * 10U;

    while ((uint16_t)(timer->instance->CNT - start_tick) < wait_ticks);
    if (was_disabled) timer->instance->CR1 &= ~TIM_CR1_CEN;
}

void timer_general_set_duty(const timer_general_t *timer, uint8_t channel, uint32_t duty_percent) {
    if (duty_percent > 100) duty_percent = 100;
    uint32_t ccr_val = ((timer->instance->ARR + 1U) * duty_percent) / 100U;
    switch (channel) {
        case 1: timer->instance->CCR1 = ccr_val; break;
        case 2: timer->instance->CCR2 = ccr_val; break;
        case 3: timer->instance->CCR3 = ccr_val; break;
        case 4: timer->instance->CCR4 = ccr_val; break;
    }
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