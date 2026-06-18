#include "stm32f446xx.h"
#include "system_stm32f4xx.h"
#include <i2c.h>
#include <stdint.h>
#include <assert.h>

static void i2c_enable_clock(i2c_t *i2c) {
    if(i2c->bus == I2C1) { RCC->APB1ENR |= RCC_APB1ENR_I2C1EN; }
    else if(i2c->bus == I2C2) { RCC->APB1ENR |= RCC_APB1ENR_I2C2EN; }
    else if((i2c->bus == I2C3)) { RCC->APB1ENR |= RCC_APB1ENR_I2C3EN; }
    (void)RCC->APB1ENR;
}

i2c_status_t i2c_init(i2c_t *i2c, uint32_t pclk1_mhz, i2c_mode_t mode) {
    assert(i2c != NULL);
    assert(pclk1_mhz >= 2U && pclk1_mhz <= 50U);
    // RCC
    i2c_enable_clock(i2c);
    // reset and off I2C before setup
    i2c->bus->CR1 &=  ~I2C_CR1_PE;
    // setup I2C bus freq
    i2c->bus->CR2 &= ~I2C_CR2_FREQ;
    i2c->bus->CR2 |= (pclk1_mhz & I2C_CR2_FREQ);     // APB1 Mhz
    // reset CCR & TRISE
    i2c->bus->CCR &= ~(I2C_CCR_FS | I2C_CCR_DUTY | I2C_CCR_CCR);
    i2c->bus->TRISE &= ~I2C_TRISE_TRISE;

    if(mode == I2C_MODE_STANDARD_100KHZ) {
        // Standard Mode (100 kHz)
        // CCR = F_pclk1 / (2 * 100 000) -> multiple 1 000 000 and then devide by 1000
        uint32_t ccr_val = (pclk1_mhz * 1000U) / 200U;
        if (ccr_val < 4) ccr_val = 4; // The minimum allowed value is 0x04, except when Duty = 1 (c)
        i2c->bus->CCR |= (ccr_val & I2C_CCR_CCR);
        // Calculate TRISE
        // TRISE = (1000ns / pclk1_mhz) + 1
        i2c->bus->TRISE |= ((pclk1_mhz + 1U) & I2C_TRISE_TRISE);
    } else {
        // Fast Mode (400 kHz)
        // Enable Fast mode bit and setup DUTY = 0. t_low/t_high = 2
        i2c->bus->CCR |= I2C_CCR_FS;
        // For DUTY = 0: CCR = F_pclk1 / (3 * 400 000)
        uint32_t ccr_val = (pclk1_mhz * 1000U) / 1200U;
        if (ccr_val < 1) ccr_val = 1;
        i2c->bus->CCR |= (ccr_val & I2C_CCR_CCR);
        // TRISE: For Fast Mode max time rise 300ns
        // (300ns * F_pclk1) + 1 = (0.3 * pclk1_mhz) + 1
        uint32_t trise_val = ((pclk1_mhz * 300U) / 1000U) + 1U;
        i2c->bus->TRISE |= (trise_val & I2C_TRISE_TRISE);
    }
    // Enable I2C
    i2c->bus->CR1 |= I2C_CR1_PE;

    return I2C_OK;
}

i2c_status_t i2c_write(i2c_t *i2c, uint8_t addr, uint8_t *data, size_t len) {
    // check if it busy
    if(i2c->status == I2C_BUSY || i2c->bus->SR2 & I2C_SR2_BUSY) {
        return I2C_BUSY;
    }

    // init data transmission context
    i2c->slave_addr = addr;
    i2c->tx_buffer = data;
    i2c->tx_len = len; 
    i2c->tx_count = 0U;
    i2c->status = I2C_BUSY;
}