#ifndef I2C_H
#define I2C_H

#include <stdint.h>
#include <stdbool.h>
#include <stm32f446xx.h>
#include <stddef.h>

// status errors
typedef enum {
    I2C_OK,
    I2C_TIMEOUT,
    I2C_NACK,
    I2C_BUS_ERROR,
    I2C_ARBITRATION_LOST,
    I2C_INVALID_ARG,
    I2C_IS_BUSY
} i2c_status_t;

// FSM for I2c
typedef enum {
    I2C_IDLE,
    I2C_START,
    I2C_ADDR,
    I2C_TX,
    I2C_RX,
    I2C_STOP,
    I2C_DONE,
    I2C_ERROR
} i2c_state_t;

// context
typedef struct {
    uint16_t addr;
    uint8_t *tx_buff;
    size_t tx_len;
    uint8_t *rx_buff;
    size_t rx_len;
    bool repeated_start;
} i2c_transaction_t;


// callback
typedef void (*i2c_callback_t)(i2c_status_t status);

// i2c obj
typedef struct {
    I2C_TypeDef *bus;               // I2C 1-2-3
    volatile i2c_state_t state;     // current state
    volatile i2c_status_t status;   // OK, NACK
    i2c_transaction_t ctx;          // current context
    uint16_t tx_cnt;                // current tx index
    uint16_t rx_cnt;                // current rx index
    volatile bool busy;
    i2c_callback_t callback;
} i2c_t;

// i2c mode
typedef enum {
    I2C_MODE_STANDARD_100KHZ,
    I2C_MODE_FAST_400KHZ
} i2c_mode_t;




// init
i2c_status_t i2c_init(i2c_t *i2c, uint32_t pclk1_mhz, i2c_mode_t mode);
// operations
i2c_status_t i2c_execute(i2c_t *i2c, i2c_transaction_t *tr);
void I2Cx_EV_IRQ_execute(i2c_t *i2c);
void I2Cx_ER_IRQ_execute(i2c_t *i2c);
#endif