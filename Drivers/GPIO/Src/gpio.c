#include "gpio.h"
#include <stdint.h>
#include <stdbool.h>
#include "stm32f446xx.h"
#include <assert.h>


static void gpio_enable_clock(GPIO_TypeDef *port) {
    if(port == GPIOA)      RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;
    else if(port == GPIOB) RCC->AHB1ENR |= RCC_AHB1ENR_GPIOBEN;
    else if(port == GPIOC) RCC->AHB1ENR |= RCC_AHB1ENR_GPIOCEN;
    else if(port == GPIOD) RCC->AHB1ENR |= RCC_AHB1ENR_GPIODEN;
    else if(port == GPIOH) RCC->AHB1ENR |= RCC_AHB1ENR_GPIOHEN;
}

void gpio_init(gpio_t gpio, gpio_mode_t mode, gpio_pull_t pull, gpio_otype_t otype, gpio_speed_t speed) {
    assert(gpio.pin <= 15);
    assert(gpio.port != NULL);
    // RCC
    gpio_enable_clock(gpio.port);
    uint32_t pin_pos = gpio.pin * 2U;
    // Set PIN mode
    gpio.port->MODER &= ~(3U << pin_pos);
    gpio.port->MODER |= ((uint32_t)mode << pin_pos);
    // Pull
    gpio.port->PUPDR &= ~(3U << pin_pos);
    gpio.port->PUPDR |= ((uint32_t)pull << pin_pos);
    // Output type (requiered 1 bit)
    gpio.port->OTYPER &= ~(1U << gpio.pin);
    gpio.port->OTYPER |= ((uint32_t)otype << gpio.pin);
    // Output speed
    gpio.port->OSPEEDR &= ~(3U << pin_pos);
    gpio.port->OSPEEDR |= ((uint32_t)speed << pin_pos);
}

void gpio_init_input(gpio_t gpio, gpio_pull_t pull) {
    assert(gpio.pin <= 15);
    assert(gpio.port != NULL);
    // RCC
    gpio_enable_clock(gpio.port);
    uint32_t pin_pos = gpio.pin * 2U;
    // Mode: Input (00)
    gpio.port->MODER &= ~(3U << pin_pos);
    // Pull
    gpio.port->PUPDR &= ~(3U << pin_pos);
    gpio.port->PUPDR |= ((uint32_t)pull << pin_pos);
}

void gpio_init_output(gpio_t gpio, bool push_pull) {
    assert(gpio.pin <= 15);
    assert(gpio.port != NULL);
    // RCC
    gpio_enable_clock(gpio.port);
    uint32_t pin_pos = gpio.pin * 2U;
    // Mode: Output (01)
    gpio.port->MODER &= ~(3U << pin_pos);
    gpio.port->MODER |= (1U << pin_pos);
    // Output type: Push-Pull (0) or Open-Drain (1)
    if (push_pull) {
        gpio.port->OTYPER &= ~(1U << gpio.pin);
    } else {
        gpio.port->OTYPER |= (1U << gpio.pin);
    }
    // Default speed MEDIUM
    gpio.port->OSPEEDR &= ~(3U << pin_pos);
    gpio.port->OSPEEDR |= (1U << pin_pos);
}

void gpio_set_alternate_function(gpio_t gpio, uint8_t af_num) {
    assert(gpio.pin <= 15);
    assert(gpio.port != NULL);
    assert(af_num <= 15);
    uint8_t reg_idx = gpio.pin >> 3U; // (pin // 8), if 0..7 = 0, if 8..15 = 1
    uint8_t bit_pos = (gpio.pin & 0x07U) * 4U; // pin % 0x07 * 4byte. 10pin & 0x07 = 2 -> 2 * 4 = 8 -> 8,9,10,11 
    gpio.port->AFR[reg_idx] &= ~(0x0FU << bit_pos);
    gpio.port->AFR[reg_idx] |= ((uint32_t)af_num << bit_pos);
}

