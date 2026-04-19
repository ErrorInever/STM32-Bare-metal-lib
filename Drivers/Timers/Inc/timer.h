#ifndef TIMER_H_
#define TIMER_H_

#ifdef __cplusplus
extern "C" {
#endif


#include <stdint.h>
#include "stm32f446xx.h"
#include <assert.h>
#include <stddef.h>


// Object Timer
typedef struct {
    TIM_TypeDef *instance;      // TIM6 or TIM7
    uint32_t bus_freq;     // APB1 freq Mhz
} timer_basic_t;

// Callback
typedef void (*timer_basic_callback_t)(void);
// Register callback to timer
void timer_basic_set_callback(TIM_TypeDef *instance, timer_basic_callback_t callback);
// Init
void timer_basic_init_ms(timer_basic_t timer, uint32_t period_ms);

#ifdef __cplusplus
}
#endif

#endif
