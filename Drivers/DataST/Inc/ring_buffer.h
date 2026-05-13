#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>


typedef struct {
    uint8_t *buffer;        // buffer memory
    size_t element_size;    // size of type
    size_t bitmask;         // bit mask for '&' operation (buff size - 1)
    volatile size_t head;   // index of head
    volatile size_t tail;   // index of tail
} RingBuffer_t;

void ring_buffer_init(RingBuffer_t *buffer, void *storage, size_t size, size_t element_size);
void ring_buffer_push(RingBuffer_t *buffer, const void *data);
void ring_buffer_push_safe(RingBuffer_t *buffer, const void *data);
void ring_buffer_pop(RingBuffer_t *buffer, void *out_data);
void ring_buffer_peek(const RingBuffer_t *buffer, void *out_data);

static inline bool is_full(const RingBuffer_t *buffer) {
    return ((buffer->head + 1) & buffer->bitmask) == buffer->tail;
}

static inline bool is_empty(const RingBuffer_t *buffer) {
    return buffer->head == buffer->tail;
}
