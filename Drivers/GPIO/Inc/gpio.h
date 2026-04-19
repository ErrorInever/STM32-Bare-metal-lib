#ifndef GPIO_H_
#define GPIO_H_

#ifdef __cplusplus
extern "C" {
#endif


#include <assert.h>
#include <stdint.h>
#include <stdbool.h>
#include "stm32f446xx.h"

// GPIO object
typedef struct {
    GPIO_TypeDef *port;   // ports: A, B, C
    uint16_t pin;         // 0..15 (bit index)
} gpio_t;

// Pin modes
typedef enum {
    GPIO_MODE_INPUT_t  = 0x00,
    GPIO_MODE_OUTPUT_t = 0x01,
    GPIO_MODE_AF_t     = 0x02,
    GPIO_MODE_ANALOG_t = 0x03
} gpio_mode_t;

// Pull modes
typedef enum {
    GPIO_PULL_NONE_t = 0x00,
    GPIO_PULL_UP_t   = 0x01,
    GPIO_PULL_DOWN_t = 0x02
} gpio_pull_t;

// OTYPER (Output Type Register)
typedef enum {
    GPIO_OTYPE_PP_t = 0x00,  // Push-pull
    GPIO_OTYPE_OD_t = 0x01   // Open-drain
} gpio_otype_t;

// OSPEEDR
typedef enum {
    GPIO_SPEED_LOW_t       = 0x00,
    GPIO_SPEED_MEDIUM_t    = 0x01,
    GPIO_SPEED_HIGH_t      = 0x02,
    GPIO_SPEED_VERY_HIGH_t = 0x03
} gpio_speed_t;

// Interrupt edges types
typedef enum {
    EDGE_RISING_t,
    EDGE_FALLING_t,
    EDGE_BOTH_t
} edge_type_t;

// Callback function type
typedef void (*gpio_callback_t)(uint16_t pin);

// GPIO INIT
//
// Universal pin init function
void gpio_init(gpio_t gpio, gpio_mode_t mode, gpio_pull_t pull, gpio_otype_t otype, gpio_speed_t speed);
// Simplified setup of a pin as an input.
void gpio_init_input(gpio_t gpio, gpio_pull_t pull);
// Simplified setup of a pin as an output. 
void gpio_init_output(gpio_t gpio, bool push_pull);
// Set alternate function
void gpio_set_alternate_function(gpio_t gpio, uint8_t af_num);
// Enable interrupts
void gpio_enable_irq(gpio_t gpio, edge_type_t edge);
// Set callback function for interrupt
void gpio_set_callback(uint16_t pin, gpio_callback_t callback);

// Fast opetations
//
// Writes a logic level to a pin. The point is to set the exit state.
static inline void gpio_write(gpio_t gpio, uint8_t state) {
    if(state) {
        gpio.port->BSRR = (1U << gpio.pin);
    } else {
        gpio.port->BSRR = (1U << (gpio.pin + 16U));
    }
}
// Reads the current logic level. The point is to get the actual state of the line.
static inline uint8_t gpio_read(gpio_t gpio) {
    return (gpio.port->IDR & (1U << gpio.pin)) ? 1 : 0;
}
// Sets the pin to 1. The point is to quickly enable the PIN
static inline void gpio_set(gpio_t gpio) {
    gpio.port->BSRR = (1U << gpio.pin);
} 
// Resets the pin to 0. The point is to quickly turn off the pin
static inline void gpio_reset(gpio_t gpio) {
    gpio.port->BSRR = (1U << (gpio.pin + 16U));
}
// Inverts the current state of the output. The point is to switch the pin without knowing the current state from the outside.
static inline void gpio_toggle(gpio_t gpio) {
    gpio.port->ODR ^= (1U << gpio.pin);
}

#ifdef __cplusplus
}
#endif

#endif
