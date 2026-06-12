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
    RingBuffer_t *rx_buffer;    // pointer to ringbuffer struct
} usart_t;

// Callback for USART
typedef void (*usart_callback_t)(void *context);
void usart_set_callback(const USART_TypeDef *usart, usart_callback_t callback, void *context);
// Init USART
void usart_init(usart_t *usart, uint32_t baudrate);

#endif
