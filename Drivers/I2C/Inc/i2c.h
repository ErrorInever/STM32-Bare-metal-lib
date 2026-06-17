#ifndef I2C_H
#define I2C_H

#include <stdint.h>
#include <stdbool.h>
#include <stm32f446xx.h>

// i2c obj
typedef struct {
    I2C_TypeDef *bus;
    uint8_t *buff;
    size_t len;
    size_t pos;
    volatile bool busy;

} i2c_t;

// status
typedef enum {
    I2C_OK,
    I2C_NACK,
    I2C_BUSY,
    I2C_TIMEOUT
} i2c_status_t;

// init
i2c_status_t i2c_init(i2c_t *i2c, uint32_t speed_hz);

// operations
i2c_status_t i2c_write(i2c_t *i2c, uint8_t addr, uint8_t *data, size_t len);

i2c_status_t i2c_read(i2c_t *i2c, uint8_t addr, uint8_t *data, size_t len);

#endif