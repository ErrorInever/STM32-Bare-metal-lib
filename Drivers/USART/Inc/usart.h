#ifndef USART_H_
#define USART_H_

#include <stdint.h>
#include "stm32f446xx.h"
#include "ring_buffer.h"
#include <assert.h>

typedef struct {
    USART_TypeDef *instance;    // USART1, USART2, USART3, USART6
    uint32_t baudrate;          // speed
    uint32_t bus_freq;          // freq of bus: USART1, USART6 = APB2,  USART2, USART3, = APB1
    RingBuffer_t *rx_buffer;    // ring buffer for recive
    RingBuffer_t *tx_buffer;    // ring buffer for transmit
} usart_t;

void usart_init(usart_t *usart, uint32_t baudrate);
void usart_send(const usart_t *usart, const char *data);
void usart_printf(const usart_t *usart, const char *format, ...);

static inline uint8_t usart_recive_byte(const USART_TypeDef *instance) {
    return instance->DR & 0xFF;
}

#endif
