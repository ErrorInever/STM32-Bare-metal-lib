#ifndef GPIO_H_
#define GPIO_H_

#include <stdint.h>
#include <stdbool.h>
#include "stm32f446xx.h"

typedef struct {
    GPIO_TypeDef *port;
    uint16_t pin;
} gpio_t;

// Interrupt types
typedef enum {
    EDGE_RISING,
    EDGE_FALLING,
    EDGE_BOTH
} edge_type_t;

// Pull types
typedef enum {
    GPIO_NO_PULL = 0,
    GPIO_PULL_UP,
    GPIO_PULL_DOWN
} gpio_pull_t;


// GPIO INIT
void gpio_init_input(gpio_t pin, gpio_pull_t pull);
void gpio_init_output(gpio_t pin, bool push_pull);
// Work with GPIO
void gpio_write(gpio_t pin, bool value);
void gpio_read(gpio_t pin);
// Toggle
void gpio_toggle(gpio_t pin);
// Callbacks
typedef void (*gpio_callback_t)(gpio_t pin);
void gpio_set_callback(gpio_t pin, gpio_callback_t cb);
// Interrupt subsystem
void gpio_attach_interrupt(gpio_t pin, edge_type_t edge);
#endif
