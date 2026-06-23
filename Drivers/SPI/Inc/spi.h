#ifndef SPI_H
#define SPI_H

#include <stdint.h>
#include <stm32f446xx.h>

struct spi_t;
struct spi_transaction_t;

typedef struct spi_t spi_t;
typedef struct spi_transaction_t spi_transaction_t;

typedef enum {
    SPI_OK,
    SPI_BUSY,
    SPI_TR_ERROR,
    SPI_OVR,
    SPI_MODF
} spi_status_t;

typedef enum {
    SPI_READY,          // bus is ready
    SPI_BUSY_TX,        // data transfer in progress
    SPI_BUSY_RX,        // data is being received
    SPI_BUSY_TX_RX,     // full duplex exchange
    SPI_ERROR           // an error occurred
} spi_state_t;

// user callback
typedef void (*spi_callback_t)(spi_t *spi, volatile spi_transaction_t *tr, spi_status_t status_tr);

typedef struct spi_transaction_t {
    const uint8_t *tx_buff;
    uint16_t tx_len;
    uint8_t *rx_buff;
    uint16_t rx_len;
    spi_callback_t callback;
} spi_transaction_t;

typedef struct spi_t {
    SPI_TypeDef *instance;
    volatile spi_state_t state;
    volatile spi_transaction_t *current_transaction;
    // pointers for interrupt
    volatile uint8_t *p_tx_ptr;
    volatile uint8_t *p_rx_ptr;
    volatile uint16_t tx_cnt;   // How many bytes are left to send
    volatile uint16_t rx_cnt;   // How many bytes are left to receive
} spi_t;

spi_status_t spi_init(spi_t *spi); 
spi_status_t spi_execute_transaction(spi_t *spi, spi_transaction_t *tr);

#endif
