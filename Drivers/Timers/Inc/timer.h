/**
 * @file timer.h
 * @brief Simple Hardware Timer Library for STM32F446xx MCUs.
 * * Provides a lightweight abstraction layer for basic timers (TIM6/TIM7) 
 * for delays and periodic interrupts, and general-purpose timers (TIM2/TIM3) 
 * for basic PWM generation.
 * * @author ErrorInever
 * @date 2026-06-17
 */


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


/**
 * @struct timer_basic_t
 * @brief Configuration structure for Basic Timers (TIM6 or TIM7).
 */
typedef struct {
    TIM_TypeDef *instance;      // TIM6 or TIM7
    uint32_t bus_freq;          // APB1 freq Mhz
    bool status;
} timer_basic_t;


/**
 * @struct timer_general_t
 * @brief Configuration structure for General Purpose Timers (e.g., TIM2 or TIM3).
 */
typedef struct {
    TIM_TypeDef *instance;      // TIM2 or TIM3
    uint32_t bus_freq;
    bool status;
} timer_general_t;

/**
 * @brief Callback function pointer type for basic timer interrupts.
 * @param[in] context User-defined pointer to pass state or data to the handler.
 */
typedef void (*timer_basic_callback_t)(void *context);

/**
 * @brief Callback function pointer type for general timer interrupts.
 * @param[in] context User-defined pointer to pass state or data to the handler.
 */
typedef void (*timer_general_callback_t)(void *context);


/* ========================================================================== */
/* Callback Registration                             */
/* ========================================================================== */

/**
 * @brief Registers an ISR callback for a specific basic timer instance.
 * * @param[in] timer    Pointer to the basic timer configuration structure.
 * @param[in] callback Function pointer to execute inside the IRQ handler.
 * @param[in] context  Pointer to context data passed to the callback function.
 */
void timer_basic_set_callback(const timer_basic_t *timer, timer_basic_callback_t callback, void *context);

/**
 * @brief Registers an ISR callback for a specific general timer instance.
 * * @note Currently, the source implementation passes a `timer_basic_t` pointer.
 * @param[in] timer    Pointer to the timer configuration structure.
 * @param[in] callback Function pointer to execute inside the IRQ handler.
 * @param[in] context  Pointer to context data passed to the callback function.
 */
void timer_general_set_callback(const timer_basic_t *timer, timer_general_callback_t callback, void *context);


/* ========================================================================== */
/* Initialization                                */
/* ========================================================================== */

/**
 * @brief Initializes a basic timer with a specific update period in milliseconds.
 * * Sets up the prescaler to step at a 10 kHz resolution (0.1ms per tick) 
 * and configures the Update Interrupts via the NVIC.
 * * @pre The `bus_freq` field must be valid between 1 and 184 MHz.
 * @param[in] timer     Pointer to the basic timer configuration structure.
 * @param[in] period_ms Target timeout period in milliseconds.
 */
void timer_basic_init_ms(const timer_basic_t *timer, uint32_t period_ms);

/**
 * @brief Initializes a general-purpose timer for PWM generation.
 * * Sets the prescaler to step at a 10 kHz resolution and calculates the base
 * period according to the desired frequency.
 * * @pre The `bus_freq` field must be valid between 1 and 184 MHz.
 * @param[in] timer       Pointer to the general timer configuration structure.
 * @param[in] pwm_freq    Target PWM base frequency.
 * @param[in] dc_percent  Initial duty cycle percentage (0 to 100).
 */
void timer_general_init_ms(const timer_general_t *timer, uint32_t period_ms, uint32_t dc_percent);

/**
 * @brief Configures a specific channel of a general-purpose timer for PWM Output Mode 1.
 * * Enables Output Compare preload capabilities and updates Output Enable bits.
 * * @param[in] timer   Pointer to the general timer configuration structure.
 * @param[in] channel Hardware channel number (1 to 4).
 * * @note *Bug Warning:* Your current implementation misses a `break;` statement 
 * at the end of `case 2:`, meaning channel 2 setup falls through into channel 3.
 */
void timer_general_pwm_channel_config(const timer_general_t *timer, uint8_t channel);


/* ========================================================================== */
/* Timer Operations                               */
/* ========================================================================== */

/**
 * @brief Starts the counting mechanism for a basic timer.
 * @param[in] timer Pointer to the basic timer configuration structure.
 */
static inline void timer_basic_start(const timer_basic_t *timer) {
    timer->instance->CR1 |= TIM_CR1_CEN;
}

/**
 * @brief Starts the counting mechanism for a general timer.
 * @param[in] timer Pointer to the general timer configuration structure.
 */
static inline void timer_general_start(const timer_general_t *timer) {
    timer->instance->CR1 |= TIM_CR1_CEN;
}


/**
 * @brief Stops the counting mechanism and clears pending update flags for a basic timer.
 * @param[in] timer Pointer to the basic timer configuration structure.
 */
static inline void timer_basic_stop(const timer_basic_t *timer) {
    timer->instance->CR1 &= ~TIM_CR1_CEN;
    timer->instance->SR &= ~TIM_SR_UIF;
}

/**
 * @brief Stops the counting mechanism and clears pending update flags for a general timer.
 * @param[in] timer Pointer to the general timer configuration structure.
 */
static inline void timer_general_stop(const timer_general_t *timer) {
    timer->instance->CR1 &= ~TIM_CR1_CEN;
    timer->instance->SR &= ~TIM_SR_UIF;
}


/**
 * @brief Fetches the user-defined software state status flag for a basic timer.
 * @param[in] timer Pointer to the basic timer configuration structure.
 * @return true if marked active, false otherwise.
 */
static inline bool timer_basic_get_status(const timer_basic_t *timer) {
    return timer->status;
}

/**
 * @brief Fetches the user-defined software state status flag for a general timer.
 * @param[in] timer Pointer to the general timer configuration structure.
 * @return true if marked active, false otherwise.
 */
static inline bool timer_general_get_status(const timer_general_t *timer) {
    return timer->status;
}


/**
 * @brief Configures One-Pulse Mode (OPM) on a basic timer.
 * * If enabled, the counter stops automatically at the next update event.
 * * @param[in] timer  Pointer to the basic timer configuration structure.
 * @param[in] enable Pass true to enable One-Pulse Mode, false for repetitive mode.
 */
static inline void timer_basic_set_one_pulse(const timer_basic_t *timer, bool enable) {
    if (enable) timer->instance->CR1 |= TIM_CR1_OPM;
    else timer->instance->CR1 &= ~TIM_CR1_OPM;
}


/**
 * @brief Dynamically changes the auto-reload period value for a basic timer.
 * * Updates the ARR register and forces a software update execution to apply shadow registers.
 * * @param[in] timer     Pointer to the basic timer configuration structure.
 * @param[in] period_ms New target period in milliseconds.
 */
static inline void timer_basic_set_period_ms(const timer_basic_t *timer, uint32_t period_ms) {
    timer->instance->ARR = (period_ms * 10) - 1; // Update ARR
    timer->instance->EGR |= TIM_EGR_UG; // forced update shadow registers
    timer->instance->SR &= ~TIM_SR_UIF;
}


/**
 * @brief Executes a blocking, synchronous millisecond delay via polling.
 * * @note This function temporarily activates the timer if it wasn't already running, 
 * and queries the internal `CNT` register value via delta calculations.
 * * @param[in] timer Pointer to the basic timer configuration structure.
 * @param[in] ms    Duration of delay block in milliseconds.
 */
void timer_basic_delay_ms(const timer_basic_t *timer, uint32_t ms);


/**
 * @brief Dynamically adjusts the duty cycle for a specific general timer PWM channel.
 * * Calculates and pushes the new compare value to the corresponding CCRx register.
 * * @param[in] timer         Pointer to the general timer configuration structure.
 * @param[in] channel       Target hardware channel (1 to 4).
 * @param[in] duty_percent  Desired duty cycle percentage value (0 to 100).
 */
void timer_general_set_duty(const timer_general_t *timer, uint8_t channel, uint32_t duty_percent);


#ifdef __cplusplus
}
#endif

#endif
