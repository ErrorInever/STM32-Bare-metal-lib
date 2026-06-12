#include "usart.h"
#include "stm32f446xx.h"
#include <stdint.h>

// Size of ringbuffer: must be a power of two
#define RING_BUF_SIZE 128

static uint8_t rx_storage[4][RING_BUF_SIZE]; // ringbuffer storage
static RingBuffer_t rb_handler[4]; // ringbuffer struct

// callbacks for IRQ
static usart_callback_t usart_handles[4];
static void *context_callback[4];

static uint8_t get_usart_index(const USART_TypeDef *usart) {
    if(usart == USART1) { return 0; }
    else if(usart == USART2) { return 1; }
    else if(usart == USART3) { return 2; }
    else if(usart == USART6) { return 3; }
    return 4;
}

void usart_set_callback(const USART_TypeDef *usart, usart_callback_t callback, void *context) {
    uint8_t usart_idx = get_usart_index(usart);
    assert(usart_idx != 4);
    usart_handles[usart_idx] = callback;
    context_callback[usart_idx] = context;
}

static void usart_enable_clock(USART_TypeDef *usart) {
    if(usart == USART1) { RCC->APB2ENR |= RCC_APB2ENR_USART1EN; }
    else if(usart == USART2) { RCC->APB1ENR |= RCC_APB1ENR_USART2EN; }
    else if(usart == USART3) { RCC->APB1ENR |= RCC_APB1ENR_USART3EN; }
    else if(usart == USART6) { RCC->APB2ENR |= RCC_APB2ENR_USART6EN; }
    (void)RCC->APB1ENR;
    (void)RCC->APB2ENR;
}

void usart_init(usart_t *usart, uint32_t baudrate) {
    assert(usart->instance != NULL);
    uint8_t idx = get_usart_index(usart->instance);
    assert(idx != 4);
    // RCC
    usart_enable_clock(usart->instance);
    // Init ringbuffer for USART
    ring_buffer_init(&rb_handler[idx], rx_storage[idx], 
        RING_BUF_SIZE, sizeof(uint8_t));
    usart->rx_buffer = &rb_handler[idx];
    usart->baudrate = baudrate;
    uint32_t div = (usart->bus_freq + (baudrate / 2U)) / baudrate;
    // Configure USART
    usart->instance->BRR = div;
    usart->instance->CR1 |= USART_CR1_TE |         // Enable transmitter
                            USART_CR1_RE |         // Enable receiver
                            USART_CR1_RXNEIE |     // Enable interrupt on receive
                            USART_CR1_UE;          // Enable USART
    // Enable IRQs

    if(usart->instance == USART1) { NVIC_EnableIRQ(USART1_IRQn); }
    else if(usart->instance == USART2) {NVIC_EnableIRQ(USART2_IRQn); }
    else if(usart->instance == USART3) {NVIC_EnableIRQ(USART3_IRQn); }
    else if(usart->instance == USART6) {NVIC_EnableIRQ(USART6_IRQn); }
}

static void generic_usart_irq_handler(USART_TypeDef *instance, uint8_t idx) {
    // Check RXNE
    if ((instance->SR & USART_SR_RXNE) && (instance->CR1 & USART_CR1_RXNEIE)) {
        uint8_t received_byte = (uint8_t)(instance->DR & 0xFF);
        
        // Send byte to buffer
        ring_buffer_push(&rb_handler[idx], &received_byte);
        
        // If you need callback each byte
        // if(usart_handles[idx]) {
        //     usart_handles[idx](context_callback[idx]);
        // }
    }
 
}


// IRQ Handlers
void USART1_IRQHandler(void) { generic_usart_irq_handler(USART1, 0); }
void USART2_IRQHandler(void) { generic_usart_irq_handler(USART2, 1); }
void USART3_IRQHandler(void) { generic_usart_irq_handler(USART3, 2); }
void USART6_IRQHandler(void) { generic_usart_irq_handler(USART6, 3); }