#include "usart.h"
#include "stm32f446xx.h"
#include <stdint.h>
#include <stdio.h>
#include <stdarg.h>

// Size of ringbuffer: must be a power of two
#define RING_BUF_SIZE 128

static uint8_t rx_storage[4][RING_BUF_SIZE];    // ringbuffer recive storage
static RingBuffer_t rx_handler[4];              // ringbuffer recive struct

static uint8_t tx_storage[4][RING_BUF_SIZE];    // ringbuffer recive storage
static RingBuffer_t tx_handler[4];              // ringbuffer recive struct


static uint8_t get_usart_index(const USART_TypeDef *usart) {
    if(usart == USART1) { return 0; }
    else if(usart == USART2) { return 1; }
    else if(usart == USART3) { return 2; }
    else if(usart == USART6) { return 3; }
    return 4;
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
    assert((usart->instance->CR1 & USART_CR1_OVER8) == 0);
    uint8_t idx = get_usart_index(usart->instance);
    assert(idx != 4);
    // RCC
    usart_enable_clock(usart->instance);
    // Init ringbuffer for USART
    ring_buffer_init(&rx_handler[idx], rx_storage[idx], 
        RING_BUF_SIZE, sizeof(uint8_t));
    usart->rx_buffer = &rx_handler[idx];
   ring_buffer_init(&tx_handler[idx], tx_storage[idx], 
        RING_BUF_SIZE, sizeof(uint8_t));
    usart->tx_buffer = &tx_handler[idx];
    // Setup baudrate
    usart->baudrate = baudrate;
    uint32_t div = (usart->bus_freq + baudrate/2) / baudrate;        // valid only for OVER8=0
    // Configure USART
    usart->instance->BRR = div;
    usart->instance->CR1 |= USART_CR1_TE |                           // Enable transmitter
                            USART_CR1_RE |                           // Enable receiver
                            USART_CR1_RXNEIE |                       // Enable interrupt on receive
                            USART_CR1_UE;                            // Enable USART
    usart->instance->CR1 &= ~USART_CR1_TXEIE;

    // Enable IRQs
    if(usart->instance == USART1) { NVIC_EnableIRQ(USART1_IRQn); }
    else if(usart->instance == USART2) {NVIC_EnableIRQ(USART2_IRQn); }
    else if(usart->instance == USART3) {NVIC_EnableIRQ(USART3_IRQn); }
    else if(usart->instance == USART6) {NVIC_EnableIRQ(USART6_IRQn); }
}

void usart_send(const usart_t *usart, const char *str) {
    uint8_t idx = get_usart_index(usart->instance);
    ENTER_CRITICAL();
    while(*str) {
        char ch = *str++;
        ring_buffer_push(&tx_handler[idx], &ch);
        
    }
    usart->instance->CR1 |= USART_CR1_TXEIE;
    EXIT_CRITICAL();
}

void usart_printf(const usart_t *usart, const char *format, ...) {
    char temp_str[128]; // temp buffer for formatted str
    va_list args;
    va_start(args, format);
    // Formatting the string into our temporary buffer
    int len = vsnprintf(temp_str, sizeof(temp_str), format, args);
    va_end(args);

    if (len < 0)
        return;

    if (len >= sizeof(temp_str))
        len = sizeof(temp_str) - 1;

    if(len > 0) {
        // Push each character into the ring buffer
        ENTER_CRITICAL();
        for (int i = 0; i < len; i++) {
            ring_buffer_push(usart->tx_buffer, &temp_str[i]);
        }
        usart->instance->CR1 |= USART_CR1_TXEIE;
        EXIT_CRITICAL();
    }
}

static void generic_usart_irq_handler(USART_TypeDef *instance, uint8_t idx) {
    // error flags
    if(instance->SR & (USART_SR_ORE || USART_SR_NE | USART_SR_FE | USART_SR_PE)) {
        volatile uint32_t dummy;
        dummy = instance->SR;       // read SR
        dummy = instance->DR;       // read DR (reset error flags)
        (void)dummy;
    }
    // If data is ready to read i.e. RXNE = 1 AND RXNEIE (Enabled interrupt)
    if ((instance->SR & USART_SR_RXNE) && (instance->CR1 & USART_CR1_RXNEIE)) {
        uint8_t received_byte = usart_recive_byte(instance);
        // TODO add overflow lost data printf
        ring_buffer_push(&rx_handler[idx], &received_byte);
    // If DR register is empty i.e. TXE = 1 AND TXEIE
    } else if((instance->SR & USART_SR_TXE) && (instance->CR1 & USART_CR1_TXEIE)) {
        // send byte
        uint8_t out_data;
        if(ring_buffer_pop(&tx_handler[idx], &out_data)) {
            instance->DR = out_data;
        } else {
            instance->CR1 &= ~USART_CR1_TXEIE; // buffer is empty, reset TXEIE flag
        }
    }
}

// IRQ Handlers
void USART1_IRQHandler(void) { generic_usart_irq_handler(USART1, 0); }
void USART2_IRQHandler(void) { generic_usart_irq_handler(USART2, 1); }
void USART3_IRQHandler(void) { generic_usart_irq_handler(USART3, 2); }
void USART6_IRQHandler(void) { generic_usart_irq_handler(USART6, 3); }