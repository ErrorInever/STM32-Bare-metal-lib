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

spi_status_t spi_init(spi_t *spi) {
    assert(spi != NULL);
    // RCC
    spi_enable_clock(spi);
    // reset and configure SPI
    spi->instance->CR1 = 0U;
    spi->instance->CR1 |= (SPI_CR1_MSTR             // we are master, MSTR = 1
                        | (1 << SPI_CR1_BR_Pos)    // freq devider by 4
                        | SPI_CR1_CPOL              // waiting for bus at 1, CPOL = 1
                        | SPI_CR1_CPHA              // second edge sampling, CPHA = 1
                        | SPI_CR1_SSM               // manual control CS, SSM = 1
                        | SPI_CR1_SSI);             // need set to 1 for manual control SSM
    
    spi->instance->CR1 |= SPI_CR1_SPE;
    
    return SPI_OK;
}

spi_status_t spi_execute_transaction(spi_t *spi, spi_transaction_t *tr) {
    assert(spi != NULL);
    assert(tr != NULL);

    if(spi->state != SPI_READY) {
        return SPI_BUSY;
    } 
    // inite transaction
    spi->current_transaction = tr;
    spi->p_tx_ptr = (uint8_t *)tr->tx_buff;
    spi->p_rx_ptr = tr->rx_buff;
    spi->tx_cnt = tr->tx_len;
    spi->rx_cnt = tr->rx_len;

    // reset interrupt flags
    spi->instance->CR2 &= ~(SPI_CR2_TXEIE | SPI_CR2_RXNEIE | SPI_CR2_ERRIE);

    if(tr->rx_buff == NULL) {
        spi->state = SPI_BUSY_TX;
        spi->instance->CR2 |= SPI_CR2_TXEIE;

    } else if(tr->tx_buff == NULL) {
        spi->state = SPI_BUSY_RX;
        // for rx need enable TXEIE for send dummy bytes 
        spi->tx_cnt = tr->rx_len;
        spi->rx_cnt = tr->rx_len;
        spi->instance->CR2 |= (SPI_CR2_TXEIE | SPI_CR2_RXNEIE | SPI_CR2_ERRIE);

    } else {
        spi->state = SPI_BUSY_TX_RX;
        spi->instance->CR2 |= (SPI_CR2_TXEIE | SPI_CR2_RXNEIE | SPI_CR2_ERRIE);

    }

    return SPI_OK;
}

// interrupts events and errors
void SPIx_IRQHandler(spi_t *spi) {
    uint32_t sr = spi->instance->SR;
    bool tr_finished = false;

    if(sr & SPI_SR_OVR) {
        // reset flag 
        (void)spi->instance->DR;
        (void)sr;
        spi->state = SPI_ERROR;

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