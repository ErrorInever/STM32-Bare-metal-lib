#include "spi.h"
#include "stm32f446xx.h"
#include <assert.h>
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>


#define NUM_SPI_CNT 4   // num of SPI interface count. current version have 4

static spi_t *spi_handler[NUM_SPI_CNT] = {NULL};


static void spi_enable_clock(const spi_t *spi) {
    if(spi->instance == SPI1) { RCC->APB2ENR |= RCC_APB2ENR_SPI1EN; }
    else if(spi->instance == SPI2) { RCC->APB1ENR |= RCC_APB1ENR_SPI2EN; }
    else if((spi->instance == SPI3)) { RCC->APB1ENR |= RCC_APB1ENR_SPI3EN; }
    else if((spi->instance == SPI4)) { RCC->APB2ENR |= RCC_APB2ENR_SPI4EN; }
    (void)RCC->APB2ENR;
} 

spi_status_t spi_init(spi_t *spi, spi_baudrate_t br) {
    assert(spi != NULL);
    // RCC
    spi_enable_clock(spi);
    // reset and configure SPI
    spi->instance->CR1 = 0U;
    spi->instance->CR1 |= (SPI_CR1_MSTR                         // we are master, MSTR = 1
                        | ((uint32_t)br << SPI_CR1_BR_Pos)      // freq devider
                        // | SPI_CR1_CPOL                          // waiting for bus at 1, CPOL = 1
                        // | SPI_CR1_CPHA                          // second edge sampling, CPHA = 1
                        | SPI_CR1_SSM                           // manual control CS, SSM = 1
                        | SPI_CR1_SSI);                         // need set to 1 for manual control SSM
    

    // ENABLE NVIC IRQs
    if(spi->instance == SPI1) { 
        NVIC_EnableIRQ(SPI1_IRQn); 
        spi_handler[0] = spi;
    }
    else if(spi->instance == SPI2) { 
        NVIC_EnableIRQ(SPI2_IRQn); 
        spi_handler[1] = spi;
    }
    else if(spi->instance == SPI3) { 
        NVIC_EnableIRQ(SPI3_IRQn); 
        spi_handler[2] = spi;
    }
    else if(spi->instance == SPI4) { 
        NVIC_EnableIRQ(SPI4_IRQn); 
        spi_handler[3] = spi;
    }

    spi->instance->CR1 |= SPI_CR1_SPE;
    
    return SPI_OK;
}

spi_status_t spi_execute_transaction(spi_t *spi, spi_transaction_t *tr) {
    assert(spi != NULL);
    assert(tr != NULL);

    if(spi->state != SPI_READY) {
        return SPI_BUSY;
    } 
    
    // init context
    spi->current_transaction = tr;
    spi->p_tx_ptr = (uint8_t *)tr->tx_buff;
    spi->p_rx_ptr = tr->rx_buff;

    // mask for interrupt flags
    uint32_t cr2mask = 0;

    if(tr->rx_buff == NULL) { // if we send i.e. tx mode only
        spi->state = SPI_BUSY_TX;
        spi->tx_cnt = tr->tx_len;
        spi->rx_cnt = tr->tx_len;
        cr2mask = (SPI_CR2_TXEIE | SPI_CR2_RXNEIE | SPI_CR2_ERRIE);
    } 
    else if(tr->tx_buff == NULL) { // if we read i.e. rx mode only
        spi->state = SPI_BUSY_RX;
        spi->tx_cnt = tr->rx_len; // rx len for send dummy bytes
        spi->rx_cnt = tr->rx_len;
        cr2mask = (SPI_CR2_TXEIE | SPI_CR2_ERRIE); // enable only TXEIE for sync first byte
    } 
    else {
        spi->state = SPI_BUSY_TX_RX;    // rx & tx mode
        spi->tx_cnt = tr->tx_len;
        spi->rx_cnt = tr->rx_len;
        cr2mask = (SPI_CR2_TXEIE | SPI_CR2_RXNEIE | SPI_CR2_ERRIE);
    }
    // reset old flags
    spi->instance->CR2 &= ~(SPI_CR2_TXEIE | SPI_CR2_RXNEIE | SPI_CR2_ERRIE);
    // enable interrupts
    spi->instance->CR2 |= cr2mask;
    return SPI_OK;
}

// interrupts events and errors
void SPIx_IRQHandler(spi_t *spi) {
    uint32_t sr = spi->instance->SR;
    uint32_t cr2 = spi->instance->CR2;

    // check errors
    if(cr2 & SPI_CR2_ERRIE) {
        // OVERRUN
        if(sr & SPI_SR_OVR) {
            // reset OVR
            (void)spi->instance->DR;
            (void)spi->instance->SR;
            // disable all interrupts
            spi->instance->CR2 &= ~(SPI_CR2_TXEIE | SPI_CR2_RXNEIE | SPI_CR2_ERRIE);
            spi->state = SPI_READY;
            if (spi->current_transaction && spi->current_transaction->callback) {
                spi->current_transaction->callback(spi, spi->current_transaction, SPI_OVR);
            }
            return;
        }
        // Mode Fault
        if(sr & SPI_SR_MODF) {
            // reset MODF
            spi->instance->CR1 &= ~SPI_CR1_SPE; // disable SPI module
            spi->instance->CR2 &= ~(SPI_CR2_TXEIE | SPI_CR2_RXNEIE | SPI_CR2_ERRIE);
            spi->state = SPI_READY;
            if (spi->current_transaction && spi->current_transaction->callback) {
                spi->current_transaction->callback(spi, spi->current_transaction, SPI_MODF);
            }
            return;
        }
    }
    // if get RXNE and interrupt was enabled
    if((sr & SPI_SR_RXNE) && (cr2 & SPI_CR2_RXNEIE)) {
        if(spi->rx_cnt > 0) { // if have anything to read
            if(spi->p_rx_ptr != NULL) { // if we read (rx mode)
                *spi->p_rx_ptr = spi->instance->DR;
                spi->p_rx_ptr++;
            } else { // if we just send (tx mode) we must read from DR
                volatile uint8_t dummy = spi->instance->DR;
                (void)dummy;
            }
            spi->rx_cnt--;

            // (mode RX) enable TXEIE for send dummy byte
            if(spi->state == SPI_BUSY_RX && spi->rx_cnt > 0) {
                spi->instance->CR2 |= SPI_CR2_TXEIE;
            }
        } 
    }  
    
    // if get TXE and interrupt was enabled
    if((sr & SPI_SR_TXE) && (cr2 & SPI_CR2_TXEIE)) {
        if(spi->tx_cnt > 0) { // if have anything to read
            if(spi->p_tx_ptr != NULL) { // if tx mode
                spi->instance->DR = *spi->p_tx_ptr;
                spi->p_tx_ptr++;
            } else { // if we just read (rx mode) we must send dummy bute in DR
                spi->instance->DR = 0xFF;

                // (mode RX) disable TXEIE after send dummy byte to wait for the incoming byte (RXNE)
                if(spi->state == SPI_BUSY_RX) {
                    spi->instance->CR2 &= ~SPI_CR2_TXEIE;
                    spi->instance->CR2 |= SPI_CR2_RXNEIE;
                }

            }
            spi->tx_cnt--;
        } else { // if tx_cnt == 0
            spi->instance->CR2 &= ~SPI_CR2_TXEIE;
        }
    }
    // if end of transaction
    if(spi->tx_cnt == 0 && spi->rx_cnt == 0) {
        // disable interrupts
        spi->instance->CR2 &= ~(SPI_CR2_TXEIE | SPI_CR2_RXNEIE | SPI_CR2_ERRIE);
        // waiting bus is empty
        while(spi->instance->SR & SPI_SR_BSY);
        // reset DR 
        if(spi->current_transaction->rx_buff == NULL) {
            volatile uint32_t dummy = spi->instance->DR;
            dummy = spi->instance->SR; // OVR
            (void)dummy;
        }
        spi->state = SPI_READY;
        // execute user callback
        if (spi->current_transaction && spi->current_transaction->callback) {
            spi->current_transaction->callback(spi, spi->current_transaction, SPI_OK);
        }
    }
}

void SPI1_IRQHandler(void) {
    if(spi_handler[0] == NULL) return;
    SPIx_IRQHandler(spi_handler[0]);
}

void SPI2_IRQHandler(void) {
    if(spi_handler[1] == NULL) return;
    SPIx_IRQHandler(spi_handler[1]);
}

void SPI3_IRQHandler(void) {
    if(spi_handler[2] == NULL) return;
    SPIx_IRQHandler(spi_handler[2]);
}

void SPI4_IRQHandler(void) {
    if(spi_handler[3] == NULL) return;
    SPIx_IRQHandler(spi_handler[3]);
}