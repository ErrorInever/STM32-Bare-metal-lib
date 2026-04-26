#ifndef TIMER_H_
#define TIMER_H_

#ifdef __cplusplus
extern "C" {
#endif


#include <stdint.h>
#include "stm32f446xx.h"
#include <assert.h>
#include <stddef.h>
#include <stdbool.h>


// Object Timer
typedef struct {
    TIM_TypeDef *instance;      // TIM6 or TIM7
    uint32_t bus_freq;          // APB1 freq Mhz
    bool status;
} timer_basic_t;

typedef struct {
    TIM_TypeDef *instance;      // TIM2 or TIM3
    uint32_t bus_freq;
} timer_general_t;

// Callback
typedef void (*timer_basic_callback_t)(void *context);
typedef void (*timer_general_callback_t)(void *context);
// Register callback to timer
void timer_basic_set_callback(const timer_basic_t *timer, timer_basic_callback_t callback, void *context);
void timer_general_set_callback(const timer_basic_t *timer, timer_general_callback_t callback, void *context);
// Init
void timer_basic_init_ms(const timer_basic_t *timer, uint32_t period_ms);
void timer_general_init_ms(const timer_general_t *timer, uint32_t period_ms, uint32_t dc_percent);
void timer_general_pwm_channel_config(const timer_general_t *timer, uint8_t channel);
// Operations
//
// Enable timer
static inline void timer_basic_start(const timer_basic_t *timer) {
    timer->instance->CR1 |= TIM_CR1_CEN;
}
// Disable timer
static inline void timer_basic_stop(const timer_basic_t *timer) {
    timer->instance->CR1 &= ~TIM_CR1_CEN;
    timer->instance->SR &= ~TIM_SR_UIF;
}
// Get status
static inline bool timer_get_status(const timer_basic_t *timer) {
    return timer->status;
}
// Set one-pulse mode
static inline void timer_basic_set_one_pulse(const timer_basic_t *timer, bool enable) {
    if (enable) timer->instance->CR1 |= TIM_CR1_OPM;
    else timer->instance->CR1 &= ~TIM_CR1_OPM;
}
// Change period_ms
static inline void timer_basic_set_period_ms(const timer_basic_t *timer, uint32_t period_ms) {
    timer->instance->ARR = (period_ms * 10) - 1; // Update ARR
    timer->instance->EGR |= TIM_EGR_UG; // forced update shadow registers
    timer->instance->SR &= ~TIM_SR_UIF;
}
// delay pooling
void timer_basic_delay_ms(const timer_basic_t *timer, uint32_t ms);

// set duty cycle 
void timer_general_set_duty(const timer_general_t *timer, uint8_t channel, uint32_t duty_percent);


#ifdef __cplusplus
}
#endif

#endif
