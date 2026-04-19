#include "gpio.h"
#include <stdint.h>
#include <stdbool.h>
#include "stm32f446xx.h"
#include <assert.h>
#include <stddef.h>

static gpio_callback_t gpio_callbacks[16] = {0}; // array of callback functions for each EXTI line

void gpio_set_callback(uint16_t pin, gpio_callback_t callback) {
    if (pin < 16) gpio_callbacks[pin] = callback;
}

static void handle_gpio_interrupt(uint16_t pin) {
    if(EXTI->PR & (1U << pin)) {
        EXTI->PR = (1U << pin); // reset flag
        if(gpio_callbacks[pin] != NULL) {
            gpio_callbacks[pin](pin);
        }
    }
}

// IRQHandlers
void EXTI0_IRQHandler(void) { handle_gpio_interrupt(0); }
void EXTI1_IRQHandler(void) { handle_gpio_interrupt(1); }
void EXTI2_IRQHandler(void) { handle_gpio_interrupt(2); }
void EXTI3_IRQHandler(void) { handle_gpio_interrupt(3); }
void EXTI4_IRQHandler(void) { handle_gpio_interrupt(4); }
void EXTI9_5_IRQHandler(void) { 
    for(int i = 5; i <= 9; i++) handle_gpio_interrupt(i);
}
void EXTI15_10_IRQHandler(void) { 
    for(int i = 10; i <= 15; i++) handle_gpio_interrupt(i);
}


static void gpio_enable_clock(GPIO_TypeDef *port) {
    if(port == GPIOA) { RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN; (void)RCC->AHB1ENR; }
    else if(port == GPIOB) { RCC->AHB1ENR |= RCC_AHB1ENR_GPIOBEN; (void)RCC->AHB1ENR; }
    else if(port == GPIOC) { RCC->AHB1ENR |= RCC_AHB1ENR_GPIOCEN; (void)RCC->AHB1ENR; }
    else if(port == GPIOD) { RCC->AHB1ENR |= RCC_AHB1ENR_GPIODEN; (void)RCC->AHB1ENR; }
    else if(port == GPIOH) { RCC->AHB1ENR |= RCC_AHB1ENR_GPIOHEN; (void)RCC->AHB1ENR; }
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

void gpio_enable_irq(gpio_t gpio, edge_type_t edge) {
    // TODO  GPIO -> SYSCFG -> EXTI -> NVIC.
    assert(gpio.pin <= 15);
    assert(gpio.port != NULL);
    // RCC SYSCFG
    RCC->APB2ENR |= RCC_APB2ENR_SYSCFGEN;
    (void)RCC->APB2ENR;
    // Сonnect the EXTI N line to the port M with SYSCFG
    uint8_t exticr_idx = gpio.pin >> 2U; // each pin 4 bite
    uint8_t pos = (gpio.pin & 0x03U) * 4U;
    uint32_t port_val = 0;
    if(gpio.port == GPIOA) port_val = 0U;
    else if(gpio.port == GPIOB) port_val = 1U;
    else if(gpio.port == GPIOC) port_val = 2U;
    else if(gpio.port == GPIOD) port_val = 3U;
    else if(gpio.port == GPIOH) port_val = 7U;
    SYSCFG->EXTICR[exticr_idx] &= ~(0x0FU << pos);
    SYSCFG->EXTICR[exticr_idx] |= (port_val << pos);
    // IMR Enable interrupt for EXTI N line
    EXTI->IMR |= (1U << gpio.pin);
    // Select and enable edge trigger
    if(edge == EDGE_RISING_t || edge == EDGE_BOTH_t) EXTI->RTSR  |= (1U << gpio.pin);
    if(edge == EDGE_FALLING_t || edge == EDGE_BOTH_t) EXTI->FTSR |= (1U << gpio.pin);
    // Enable interrupt in NVIC
    IRQn_Type irq_type;
    if(gpio.pin <= 4) irq_type = (IRQn_Type)(EXTI0_IRQn + gpio.pin);
    else if(gpio.pin <= 9) irq_type = EXTI9_5_IRQn;
    else irq_type = EXTI15_10_IRQn;

    NVIC_EnableIRQ(irq_type);

}