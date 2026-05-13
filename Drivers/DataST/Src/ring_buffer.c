#include "ring_buffer.h"
#include <stdint.h>
#include <string.h>
#include "stm32f4xx.h"

#define ENTER_CRITICAL() __disable_irq()
#define EXIT_CRITICAL()  __enable_irq()

void ring_buffer_init(RingBuffer_t *buffer, void *storage, size_t size, size_t element_size) {
    // check size is power of two
    buffer->buffer = (uint8_t *)storage;
    buffer->bitmask = size - 1;
    buffer->element_size = element_size;
    buffer->head = 0;
    buffer->tail = 0;
}

void ring_buffer_push(RingBuffer_t *buffer, const void *data) {
    if(is_full(buffer)) {
        buffer->tail = (buffer->tail + 1) & buffer->bitmask;
    }
    void *dest = &buffer->buffer[buffer->head * buffer->element_size];
    memcpy(dest, data, buffer->element_size);
    buffer->head = (buffer->head + 1) & buffer->bitmask;
}

void ring_buffer_push_safe(RingBuffer_t *buffer, const void *data) {
    ENTER_CRITICAL();
    ring_buffer_push(buffer, data);
    EXIT_CRITICAL();
}

void ring_buffer_pop(RingBuffer_t *buffer, void *out_data) {
    if (is_empty(buffer)) return;
    void *src = &buffer->buffer[buffer->tail * buffer->element_size];
    memcpy(out_data, src, buffer->element_size);
    buffer->tail = (buffer->tail + 1) & buffer->bitmask;
}
void ring_buffer_peek(const RingBuffer_t *buffer, void *out_data) {
    // if not empty
    void *src = &buffer->buffer[buffer->tail * buffer->element_size];
    memcpy(out_data, src, buffer->element_size);
}