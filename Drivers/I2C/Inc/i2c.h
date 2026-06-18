#ifndef I2C_H
#define I2C_H

#include <stdint.h>
#include <stdbool.h>
#include <stm32f446xx.h>
#include <stddef.h>

// status
typedef enum {
    I2C_OK,
    I2C_NACK,
    I2C_BUSY,
    I2C_TIMEOUT,
    I2C_ERROR
} i2c_status_t;

// i2c obj
typedef struct {
    I2C_TypeDef *bus;               // I2C 1-2-3
    uint8_t slave_addr;             // slave address
    uint8_t *tx_buffer;             // transmit buffer
    size_t tx_len;                  // which bytes need to send
    size_t tx_count;                // which bytes already sended
    volatile i2c_status_t status;   // current status (busy, ok, error etc)

} i2c_t;

// i2c mode
typedef enum {
    I2C_MODE_STANDARD_100KHZ,
    I2C_MODE_FAST_400KHZ
} i2c_mode_t;

// init
i2c_status_t i2c_init(i2c_t *i2c, uint32_t pclk1_mhz, i2c_mode_t mode);

// operations
i2c_status_t i2c_write(i2c_t *i2c, uint8_t addr, uint8_t *data, size_t len);

i2c_status_t i2c_read(i2c_t *i2c, uint8_t addr, uint8_t *data, size_t len);


// reset addr
static inline void i2c_reset_addr(i2c_t *i2c) {
    (void)i2c->bus->SR1;
    (void)i2c->bus->SR1;
}




#endif