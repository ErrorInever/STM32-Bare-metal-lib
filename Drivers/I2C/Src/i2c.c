#include "stm32f446xx.h"
#include "system_stm32f4xx.h"
#include <i2c.h>
#include <stdint.h>
#include <assert.h>

#define NUM_I2C 3   // number of I2C 

static i2c_t *i2c_handler[NUM_I2C] = {NULL};


static void i2c_enable_clock(const i2c_t *i2c) {
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
    // Enable I2C perif
    i2c->bus->CR1 |= I2C_CR1_PE;

    // ENABLE NVIC IRQs
    if(i2c->bus == I2C1) { 
        NVIC_EnableIRQ(I2C1_EV_IRQn); 
        NVIC_EnableIRQ(I2C1_ER_IRQn); 
        i2c_handler[0] = i2c;
    }
    else if(i2c->bus == I2C2) {
        NVIC_EnableIRQ(I2C2_EV_IRQn); 
        NVIC_EnableIRQ(I2C2_ER_IRQn); 
        i2c_handler[1] = i2c;
    }
    else if(i2c->bus == I2C3) {
        NVIC_EnableIRQ(I2C3_EV_IRQn); 
        NVIC_EnableIRQ(I2C3_ER_IRQn); 
        i2c_handler[2] = i2c;
     }
    return I2C_OK;
}

i2c_status_t i2c_execute(i2c_t *i2c, i2c_transaction_t *tr) {
    assert(i2c != NULL && tr != NULL);
    
    if(i2c->busy || (i2c->bus->SR2 & I2C_SR2_BUSY)) {
        return I2C_IS_BUSY;
    }
    
    i2c->busy = true;
    i2c->ctx = *tr;
    i2c->tx_cnt = 0; 
    i2c->rx_cnt = 0;
    i2c->status = I2C_OK; 
    i2c->state = I2C_START;

    i2c->bus->CR2 |= (I2C_CR2_ITEVTEN | I2C_CR2_ITERREN);
    i2c->bus->CR1 |= I2C_CR1_START;

    return I2C_OK;
}

void I2Cx_EV_IRQ_execute(i2c_t *i2c) {
    uint32_t sr1 = i2c->bus->SR1;
    bool transaction_finished = false;
    // I2C FSM
    switch (i2c->state) {
        case I2C_START:
            // start bit was generated successfull
            if(sr1 & I2C_SR1_SB) {
                i2c->state = I2C_ADDR;
                // reset flag and define read (0x01) or write (0x00) byte
                uint8_t raw_bit = (i2c->ctx.tx_len > 0) ? 0x00U : 0x01U;
                i2c->bus->DR = (i2c->ctx.addr << 1) | raw_bit;
            }
            break;

        case I2C_ADDR:
            // if address send successfull and get ACK from slave
            if(sr1 & I2C_SR1_ADDR) {
                (void)i2c->bus->SR2; // reset addr (read sr1 &sr2)
                // if we want to send
                if(i2c->ctx.tx_len > 0) {
                    i2c->state = I2C_TX;
                    i2c->bus->CR2 |= I2C_CR2_ITBUFEN; // Buffer Interrupt Enable TXE
                    i2c->bus->DR = i2c->ctx.tx_buff[i2c->tx_cnt++]; // send a first byte
                } else if(i2c->ctx.rx_len > 0) { // if we want to read
                    i2c->state = I2C_RX;
                    i2c->bus->CR2 |= I2C_CR2_ITBUFEN; // Buffer Interrupt Enable RXNE
                    // If we need only 1 byte, we must immediately disable ACK, in accordance with the specification.
                    if(i2c->ctx.rx_len == 1) {
                        i2c->bus->CR1 &= ~I2C_CR1_ACK;
                        i2c->bus->CR1 |= I2C_CR1_STOP;
                    } else {
                        i2c->bus->CR1 |= I2C_CR1_ACK;
                    }
                }
            }
            break;

        case I2C_TX:
            // if end transfer
            if(sr1 & I2C_SR1_BTF) {
                if(i2c->tx_cnt >= i2c->ctx.tx_len) {
                    if (i2c->ctx.repeated_start && i2c->ctx.rx_len > 0) {
                        i2c->ctx.tx_len = 0; 
                        i2c->state = I2C_START; 
                        i2c->bus->CR1 |= I2C_CR1_START;
                    } else {
                        i2c->bus->CR1 |= I2C_CR1_STOP;
                        i2c->bus->CR2 &= ~(I2C_CR2_ITEVTEN | I2C_CR2_ITERREN);
                        i2c->status = I2C_OK;
                        i2c->state = I2C_DONE;
                        i2c->busy = false;
                        transaction_finished = true;
                    }
                    break;
                }
            }
            // if BTF = 0 and DR is empty
            if(sr1 & I2C_SR1_TXE) {
                if(i2c->tx_cnt < i2c->ctx.tx_len) {
                    i2c->bus->DR = i2c->ctx.tx_buff[i2c->tx_cnt++];
                } else { 
                    // in previous step we copy last byte and now it in shift register
                    // just disable ITBUFFEN interrupt and wait BTF = 1
                    i2c->bus->CR2 &= ~I2C_CR2_ITBUFEN;
                }
            }
            break;

        case I2C_RX:
            // DR is not empty, read next byte
            if(sr1 & I2C_SR1_RXNE) {
                // Remaining number of bytes (including DR)
                size_t remaining = i2c->ctx.rx_len - i2c->rx_cnt;
                if(remaining == 2) {
                    // If we read the penultimate byte now, the shift register
                    // will start receiving the very last one. We must disable ACK in advance
                    i2c->bus->CR1 &= ~I2C_CR1_ACK;
                    // Also, according to the RM specification, if we want to generate a STOP 
                    // condition immediately after the last byte, the STOP command 
                    // can be set right now.
                    i2c->bus->CR1 |= I2C_CR1_STOP;    
                }
                i2c->ctx.rx_buff[i2c->rx_cnt++] = i2c->bus->DR; // RXNE autoreset after read DR
                // if it was the last byte stop transaction
                if(i2c->rx_cnt >= i2c->ctx.rx_len) {
                    i2c->bus->CR2 &= ~(I2C_CR2_ITBUFEN | I2C_CR2_ITEVTEN | I2C_CR2_ITERREN);
                    i2c->status = I2C_OK;
                    i2c->state = I2C_DONE;
                    i2c->busy = false;
                    transaction_finished = true;
                }
            }
            break;

        default:
            break;
    }

    if(transaction_finished && i2c->callback != NULL) {
        i2c->callback(i2c->status);
    }
}

// event interrupt

void I2C1_EV_IRQHandler(void) {
    if(i2c_handler[0] == NULL) return;
    I2Cx_EV_IRQ_execute(i2c_handler[0]);
}

void I2C2_EV_IRQHandler(void) {
    if(i2c_handler[1] == NULL) return;
    I2Cx_EV_IRQ_execute(i2c_handler[1]);
}

void I2C3_EV_IRQHandler(void) {
    if(i2c_handler[2] == NULL) return;
    I2Cx_EV_IRQ_execute(i2c_handler[2]);
}


void I2Cx_ER_IRQ_execute(i2c_t *i2c) {
    uint32_t sr1 = i2c->bus->SR1;
    if(sr1 & I2C_SR1_AF) {
        i2c->status = I2C_NACK;
        i2c->state = I2C_ERROR;
        i2c->bus->SR1 &= ~I2C_SR1_AF;
    }
    else if(sr1 & I2C_SR1_BERR) {
        i2c->status = I2C_BUS_ERROR;
        i2c->state = I2C_ERROR;
        i2c->bus->SR1 &= ~I2C_SR1_BERR;
    }
    i2c->bus->CR1 |= I2C_CR1_STOP;
    i2c->bus->CR2 &= ~(I2C_CR2_ITEVTEN | I2C_CR2_ITBUFEN | I2C_CR2_ITERREN);
    i2c->busy = false;

    if(i2c->callback != NULL) {
        i2c->callback(i2c->status);
    }
}

// error interrupt

void I2C1_ER_IRQHandler(void) {
    if(i2c_handler[0] == NULL) return;
    I2Cx_ER_IRQ_execute(i2c_handler[0]);
}

void I2C2_ER_IRQHandler(void) {
    if(i2c_handler[1] == NULL) return;
    I2Cx_ER_IRQ_execute(i2c_handler[1]);
}

void I2C3_ER_IRQHandler(void) {
    if(i2c_handler[2] == NULL) return;
    I2Cx_ER_IRQ_execute(i2c_handler[2]);    
}
