#include "gpio.h"
#include <stdint.h>
#include <stdbool.h>
#include "main.h"
#include "stm32f446xx.h"


static void gpio_enable_clock(GPIO_TypeDef *port) {
    if(port == GPIOA) 
        RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;
    else if(port == GPIOB)
        RCC->AHB1ENR |= RCC_AHB1ENR_GPIOBEN;
    else if(port == GPIOC)
        RCC->AHB1ENR |= RCC_AHB1ENR_GPIOCEN;
}

void gpio_init_input(gpio_t gpio, gpio_pull_t pull) {
    // RCC on
    gpio_enable_clock(gpio.port);
    // Set input mode (00)
    uint32_t pin_pos = gpio.pin * 2;
    gpio.port->MODER &= ~(3U << pin_pos);
    // Configure pull-up
    gpio.port->PUPDR &= ~(3U << pin_pos);
    if(pull == GPIO_PULL_UP) { 
        gpio.port->PUPDR |= (1U << pin_pos);
    } else if(pull == GPIO_PULL_DOWN) {
        gpio.port->PUPDR |= (2U << pin_pos);
    }
}