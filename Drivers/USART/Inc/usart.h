/**
 * @file usart.h
 * @brief Interrupt-driven, ring-buffered USART Driver for STM32F446xx MCUs.
 *
 * Implements non-blocking string/byte transmission and reception using
 * internal circular static ring buffers for up to four USART instances.
 * Includes formatting features similar to standard printf.
 *
 * @author ErrorInever
 * @date 2026-06-17
 */

#ifndef USART_H_
#define USART_H_

#include <stdint.h>
#include "stm32f446xx.h"
#include "ring_buffer.h"
#include <assert.h>
#include <stdbool.h>

struct usart_t;
typedef struct usart_t usart_t;

// user callback
typedef void (*usart_callback_t)(usart_t *usart, uint16_t size);

/**
 * @struct usart_t
 * @brief Handle structure representing a fully configured USART peripheral instanced entity.
 */
typedef struct usart_t {
    USART_TypeDef *instance;    /**< Pointer to hardware peripheral registers (USART1, USART2, USART3, USART6). */
    uint32_t baudrate;          /**< Desired transmission speed rate (e.g., 9600, 115200). */
    uint32_t bus_freq;          /**< Operating bus frequency in Hz. APB2 for USART1/6; APB1 for USART2/3. */
    RingBuffer_t *rx_buffer;    /**< Reference pointer to the underlying incoming circular ring buffer structure. */
    RingBuffer_t *tx_buffer;    /**< Reference pointer to the underlying outgoing circular ring buffer structure. */

    // DMA 
    DMA_TypeDef *dma_instance;
    DMA_Stream_TypeDef *dma_stream_tx;
    uint32_t dma_tx_channel;
    DMA_Stream_TypeDef *dma_stream_rx;
    uint32_t dma_rx_channel;
    uint8_t dma_rx_tail;
    usart_callback_t callback;  //**< User callback */
} usart_t;


/* ========================================================================== */
/* Initialization & Control Interface                                         */
/* ========================================================================== */

/**
 * @brief Configures, initializes, and enables the assigned USART hardware module.
 * * Automatically handles routing power through RCC, creating local memory 
 * boundaries for internal circular structures, mapping pointers, computing baud register configuration maps, 
 * and turning on necessary NVIC interrupt groups.
 * * @pre The current peripheral register pointer `instance` inside the handle must be completely valid.
 * @pre Oversampling mode by 8 must be disabled (`OVER8 = 0`).
 * * @param[in,out] usart    Pointer to user-allocated USART context handle configuration structure.
 * @param[in]     baudrate Expected operation transmission speeds (bps).
 */
void usart_init(usart_t *usart, uint32_t baudrate);

/* ========================================================================== */
/* Transmission Functions                                                    */
/* ========================================================================== */

/**
 * @brief Enqueues a null-terminated string sequentially into the background transmission queue.
 * * Enters an internal critical section to inject string components into the hardware ring buffer,
 * then flags the hardware `TXEIE` register flag to kick-off transmission out through the ISR background worker loop.
 * * @param[in] usart Pointer to a valid initialized USART context handle.
 * @param[in] data  Pointer to standard null-terminated C-string array data to transmit.
 */
void usart_send(const usart_t *usart, const char *data);

/**
 * @brief Formats and enqueues a multi-variable string context to transmit over USART.
 * * Safely acts as a wrapper mapping `vsnprintf` internally with a max stack depth allocation ceiling of 128 bytes. 
 * Converts complex strings asynchronously out through the local static driver ring buffers.
 * * @param[in] usart  Pointer to a valid initialized USART context handle.
 * @param[in] format Pointer to a standard format specifier payload layout control sequence.
 * @param[in] ...    Variable argument tracking indices matching layout declarations.
 */
void usart_printf(const usart_t *usart, const char *format, ...);


/* ========================================================================== */
/* Reception Functions                                                       */
/* ========================================================================== */

/**
 * @brief Checks if any unread data bytes are pending in the system's input ring buffer.
 * * @param[in] usart Pointer to a valid initialized USART context handle.
 * @return true if there is at least one byte waiting to be processed, false if empty.
 */
bool usart_available(const usart_t *usart);


/**
 * @brief Extracts the oldest individual processing data byte out from the local incoming buffer.
 * * @param[in]  usart    Pointer to a valid initialized USART context handle.
 * @param[out] out_data Reference memory destination mapping address to copy data elements over to.
 * @return true if extraction succeeded, false if the incoming ring buffer was completely empty.
 */
bool usart_read_byte(const usart_t *usart, uint8_t *out_data);

/* ========================================================================== */
/* Utility & Inline Helpers                                                   */
/* ========================================================================== */

/**
 * @brief Direct utility helper processing explicit test triggers sent over serial terminals.
 * * Directly tests operations parsing instructions ('1', '2', 'h') and pipes responses 
 * backwards. Automatically switches back to fallback fallback echo processing loops for unmapped instructions.
 * * @param[in] usart Pointer to a valid initialized USART context handle.
 */
void process_simple_commands(const usart_t *usart);

/**
 * @brief Inline utility mapping an immediate 8-bit read directly off the hardware data register bus structure.
 * * @param[in] instance Direct pointer targeting a valid hardware USART memory map interface register group.
 * @return Extracted raw character code byte from the lower bits of the Data Register (DR).
 */
static inline uint8_t usart_receive_byte(const USART_TypeDef *instance) {
    return instance->DR & 0xFF;
}

void usart2_rx_init_dma(usart_t *usart, uint32_t baudrate);

#endif /* USART_H_ */
