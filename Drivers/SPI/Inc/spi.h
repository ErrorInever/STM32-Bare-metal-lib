/**
 * @file spi.h
 * @brief Non-blocking SPI driver using IRQ and DMA for STM32F446xx.
 * * This library provides a low-level, asynchronous SPI master implementation 
 * using either explicit interrupt handling (IRQ mode) or high-performance DMA transfers.
 * It automatically manages half-duplex and full-duplex contexts by handling 
 * dummy reads/writes internally.
 * @author ErrorInever
 * @date 2026
 */

#ifndef SPI_H
#define SPI_H

#include <stdint.h>
#include <stm32f446xx.h>

/* Forward declarations */
struct spi_t;
struct spi_transaction_t;

typedef struct spi_t spi_t;
typedef struct spi_transaction_t spi_transaction_t;

/**
 * @enum spi_status_t
 * @brief SPI operation status and error codes reported back via callbacks or function returns.
 */
typedef enum {
    SPI_OK,         /**< Operation completed successfully or driver initialized ok. */
    SPI_BUSY,       /**< SPI hardware or state machine is currently occupied. */
    SPI_TR_ERROR,   /**< Transaction configuration or logic error. */
    SPI_OVR,        /**< Overrun error detected (Rx buffer filled before reading). */
    SPI_MODF        /**< Mode Fault error detected (Master mode collision). */
} spi_status_t;

/**
 * @enum spi_state_t
 * @brief Internal state machine statuses for tracking the ongoing bus operations.
 */
typedef enum {
    SPI_READY,          /**< Bus is idle and ready for a new transaction. */
    SPI_BUSY_TX,        /**< Transmit-only operation is in progress. */
    SPI_BUSY_RX,        /**< Receive-only operation is in progress (sending dummy bytes). */
    SPI_BUSY_TX_RX,     /**< Full-duplex bidirectional exchange is in progress. */
    SPI_ERROR           /**< Peripheral error occurred, blocking operations. */
} spi_state_t;

/**
 * @enum spi_baudrate_t
 * @brief SPI Clock Prescaler options mapping directly to the CR1->BR configuration bits.
 * @note F_sck = F_pclk / Divider.
 */
typedef enum {
    SPI_BAUDRATE_DIV2   = 0, /**< PCLK divided by 2 */
    SPI_BAUDRATE_DIV4   = 1, /**< PCLK divided by 4 */
    SPI_BAUDRATE_DIV8   = 2, /**< PCLK divided by 8 */
    SPI_BAUDRATE_DIV16  = 3, /**< PCLK divided by 16 */
    SPI_BAUDRATE_DIV32  = 4, /**< PCLK divided by 32 */
    SPI_BAUDRATE_DIV64  = 5, /**< PCLK divided by 64 */
    SPI_BAUDRATE_DIV128 = 6, /**< PCLK divided by 128 */
    SPI_BAUDRATE_DIV256 = 7  /**< PCLK divided by 256 */
} spi_baudrate_t;

/**
 * @brief Asynchronous completion callback type definition.
 * * @param spi Pointer to the SPI handle managing the operation.
 * @param tr Pointer to the volatile transaction block that just completed.
 * @param status_tr Final status code of the transaction (e.g., SPI_OK, SPI_OVR).
 */
typedef void (*spi_callback_t)(spi_t *spi, volatile spi_transaction_t *tr, spi_status_t status_tr);

/**
 * @struct spi_transaction_t
 * @brief Defines a data package configuration to be transmitted or received over the SPI bus.
 */
typedef struct spi_transaction_t {
    const uint8_t *tx_buff;  /**< Pointer to transmit buffer. Set to NULL for Rx-only mode. */
    uint16_t tx_len;        /**< Size of transmit data in bytes. */
    uint8_t *rx_buff;        /**< Pointer to receive buffer. Set to NULL for Tx-only mode. */
    uint16_t rx_len;        /**< Size of expected receive data in bytes. */
    spi_callback_t callback;/**< Optional completion callback invoked inside the ISR context. */
} spi_transaction_t;

/**
 * @struct spi_t
 * @brief Hardware-agnostic SPI master context handle containing runtime state and DMA configurations.
 */
typedef struct spi_t {
    SPI_TypeDef *instance;                     /**< Core peripheral pointer (SPI1, SPI2, SPI3, SPI4). */
    volatile spi_state_t state;                /**< Current state machine status. */
    volatile spi_transaction_t *current_transaction; /**< Reference to the currently processed transaction. */
    
    /* IRQ Context tracking variables */
    volatile uint8_t *p_tx_ptr;                /**< Internal tracking pointer for shifting Tx bytes. */
    volatile uint8_t *p_rx_ptr;                /**< Internal tracking pointer for storing Rx bytes. */
    volatile uint16_t tx_cnt;                  /**< Amount of remaining bytes to load into DR. */
    volatile uint16_t rx_cnt;                  /**< Amount of remaining bytes to read from DR. */
    
    /* Stateless DMA stream configurations */    
    DMA_TypeDef *dma_instance;                  /**< Associated DMA controller instance (DMA1 or DMA2). */
    DMA_Stream_TypeDef *dma_stream_tx;         /**< Selected DMA Stream for the TX channel. */
    uint32_t dma_tx_channel;                   /**< Specific channel value for the TX stream allocation. */
    DMA_Stream_TypeDef *dma_stream_rx;         /**< Selected DMA Stream for the RX channel. */
    uint32_t dma_rx_channel;                   /**< Specific channel value for the RX stream allocation. */
} spi_t;

/**
 * @brief Initializes the target SPI hardware block as Master with manual Software Slave Management (SSM).
 * * Automatically enables the target APB clock, clears and configures CR1 settings (MSTR, Prescaler, SSM/SSI),
 * maps internal handlers for ISR processing, and activates the required global NVIC IRQ line before turning on SPI.
 * * @param spi Pointer to an allocated SPI handle structure with a populated `instance` field.
 * @param br Clock prescaler divider selection.
 * @return SPI_OK on success.
 */
spi_status_t spi_init(spi_t *spi, spi_baudrate_t br); 

/**
 * @brief Sets up basic DMA infrastructure, stream parameters, and maps internal RX completion interrupts.
 * * Activates AHB clocking for the designated DMA controller, clears the current control configurations,
 * maps hardware target DR registers as static peripheral addresses, sets channel routings, and binds 
 * the RX stream interrupt into the local controller driver mapping table with standard priority.
 * * @param spi Pointer to a fully initialized SPI handle with assigned DMA parameters.
 * @return SPI_OK on configuration completion.
 */
spi_status_t spi_config_dma(spi_t *spi);

/**
 * @brief Launches an asynchronous, non-blocking SPI transaction using standard interrupt lines.
 * * Checks the availability of the interface, evaluates the presence of `tx_buff` and `rx_buff` inside
 * the transaction block to configure appropriate TXEIE / RXNEIE masking fields, resets previous flags, 
 * and updates internal tracking parameters.
 * * @param spi Pointer to an active SPI handle structure.
 * @param tr Pointer to the target data layout configuration to execute.
 * @retval SPI_BUSY The hardware instance is currently busy executing another payload.
 * @retval SPI_OK Interrupts were successfully scheduled.
 */
spi_status_t spi_execute_transaction(spi_t *spi, spi_transaction_t *tr);

/**
 * @brief Launches an asynchronous, non-blocking SPI transaction using high-speed DMA streaming channels.
 * * Forces full-duplex operation under the hood. If either `tx_buff` or `rx_buff` is missing, the routine
 * automatically disables memory incrementing (MINC) for that stream and maps it to a safe internal dummy byte,
 * keeping the SPI clock line cycling synchronously. The routine evaluates lengths, resets DMA status flags, 
 * turns on Transfer Complete Interrupts (TCIE) for RX, and kicks off hardware streams concurrently.
 * * @param spi Pointer to a configured, DMA-ready SPI handle structure.
 * @param tr Pointer to the target data layout configuration to execute.
 * @retval SPI_BUSY The hardware or stream instance is currently busy.
 * @retval SPI_OK The DMA transfer was successfully started.
 */
spi_status_t spi_execute_transaction_dma(spi_t *spi, spi_transaction_t *tr);

#endif /* SPI_H */