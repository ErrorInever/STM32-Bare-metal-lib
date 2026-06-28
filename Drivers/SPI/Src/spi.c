#include "spi.h"
#include "stm32f446xx.h"
#include <assert.h>
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>


#define NUM_SPI_CNT 4   // num of SPI interface count. current version have 4

static spi_t *spi_handler[NUM_SPI_CNT] = {NULL};

// for DMA dummy
static uint8_t dma_dummy_tx = 0xFF;
static volatile uint8_t dma_dummy_rx;

// DMA streams
// first 8 for DMA1 (0 .. 7), next 8 for DMA2 (0 .. 7)
static spi_t *registered_dma_tx[16] = {NULL};
static spi_t *registered_dma_rx[16] = {NULL};


static const IRQn_Type dma_irqn_map[16] = {
    DMA1_Stream0_IRQn, DMA1_Stream1_IRQn, DMA1_Stream2_IRQn, DMA1_Stream3_IRQn,
    DMA1_Stream4_IRQn, DMA1_Stream5_IRQn, DMA1_Stream6_IRQn, DMA1_Stream7_IRQn,
    DMA2_Stream0_IRQn, DMA2_Stream1_IRQn, DMA2_Stream2_IRQn, DMA2_Stream3_IRQn,
    DMA2_Stream4_IRQn, DMA2_Stream5_IRQn, DMA2_Stream6_IRQn, DMA2_Stream7_IRQn
};


static void spi_enable_clock(const spi_t *spi) {
    if(spi->instance == SPI1) { RCC->APB2ENR |= RCC_APB2ENR_SPI1EN; }
    else if(spi->instance == SPI2) { RCC->APB1ENR |= RCC_APB1ENR_SPI2EN; }
    else if((spi->instance == SPI3)) { RCC->APB1ENR |= RCC_APB1ENR_SPI3EN; }
    else if((spi->instance == SPI4)) { RCC->APB2ENR |= RCC_APB2ENR_SPI4EN; }
    (void)RCC->APB2ENR;
} 

static void dma_enable_clock(DMA_TypeDef *instance) {
    if(instance == DMA1) RCC->AHB1ENR |= RCC_AHB1ENR_DMA1EN;
    else if(instance == DMA2) RCC->AHB1ENR |= RCC_AHB1ENR_DMA2EN;
}

static int get_stream_number(DMA_Stream_TypeDef *stream) {
    if(stream == DMA1_Stream0) return 0;
    else if(stream == DMA1_Stream1) return 1;
    else if(stream == DMA1_Stream2) return 2;
    else if(stream == DMA1_Stream3) return 3;
    else if(stream == DMA1_Stream4) return 4;
    else if(stream == DMA1_Stream5) return 5;
    else if(stream == DMA1_Stream6) return 6;
    else if(stream == DMA1_Stream7) return 7;
    else if(stream == DMA2_Stream0) return 8;
    else if(stream == DMA2_Stream1) return 9;
    else if(stream == DMA2_Stream2) return 10;
    else if(stream == DMA2_Stream3) return 11;
    else if(stream == DMA2_Stream4) return 12;
    else if(stream == DMA2_Stream5) return 13;
    else if(stream == DMA2_Stream6) return 14;
    else if(stream == DMA2_Stream7) return 15;
    return -1;
}

// reset flags for stream
static void dma_clear_all_flags(DMA_TypeDef *dma, DMA_Stream_TypeDef *stream) {
    int stream_num = get_stream_number(stream); 
    if (stream_num < 0) return;

    int s = stream_num % 8; // get index of stream DMA (0..7)
    uint32_t mask = 0;

    // mask IRQ
    if (s == 0 || s == 4)      mask = 0x3DU;
    else if (s == 1 || s == 5) mask = 0x3DU << 6;
    else if (s == 2 || s == 6) mask = 0x3DU << 16;
    else if (s == 3 || s == 7) mask = 0x3DU << 22;

    if (s < 4) {
        dma->LIFCR = mask;
    } else {
        dma->HIFCR = mask;
    }
}

// Check Transfer Complete for idx stream (0..15)
static bool dma_is_tc_flag_set(DMA_TypeDef *dma, int stream_idx) {
    int s = stream_idx % 8; // get index of stream DMA (0..7)
    uint32_t sr = (s < 4) ? dma->LISR : dma->HISR;
    uint32_t mask = 0;
    
    // TCIF
    if (s == 0 || s == 4)      mask = 1U << 5;  // TCIF0 / TCIF4
    else if (s == 1 || s == 5) mask = 1U << 11; // TCIF1 / TCIF5
    else if (s == 2 || s == 6) mask = 1U << 21; // TCIF2 / TCIF6
    else if (s == 3 || s == 7) mask = 1U << 27; // TCIF3 / TCIF7
    
    return (sr & mask) != 0;
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

spi_status_t spi_config_dma(spi_t *spi) {
    assert(spi != NULL);
    // Enable DMA RCC
    dma_enable_clock(spi->dma_instance);
    // disable stream for configure DMA
    spi->dma_stream_tx->CR &= ~DMA_SxCR_EN;
    while(spi->dma_stream_tx->CR & DMA_SxCR_EN);
    spi->dma_stream_rx->CR &= ~DMA_SxCR_EN;
    while(spi->dma_stream_rx->CR & DMA_SxCR_EN);

    // add register idx for RX only
    int idx_rx = get_stream_number(spi->dma_stream_rx);
    assert(idx_rx >= 0);
    if (idx_rx >= 0) {
        registered_dma_rx[idx_rx] = spi;
    }
    NVIC_EnableIRQ(dma_irqn_map[idx_rx]);
    NVIC_SetPriority(dma_irqn_map[idx_rx], 5);


    // reset settings
    uint32_t tx_temp_cr = spi->dma_stream_tx->CR & ~(DMA_SxCR_DIR | DMA_SxCR_MINC | DMA_SxCR_PINC 
            | DMA_SxCR_MSIZE | DMA_SxCR_PSIZE | DMA_SxCR_CHSEL);
    uint32_t rx_temp_cr = spi->dma_stream_rx->CR & ~(DMA_SxCR_DIR | DMA_SxCR_MINC | DMA_SxCR_PINC 
            | DMA_SxCR_MSIZE | DMA_SxCR_PSIZE | DMA_SxCR_CHSEL);  
            
    // setup direction
    tx_temp_cr |= DMA_SxCR_DIR_0; // Memory to peripheral
    rx_temp_cr |= 0U; // peripheral to memory
    
    // static PAR addrs
    spi->dma_stream_tx->PAR = (uint32_t)&spi->instance->DR;
    spi->dma_stream_rx->PAR = (uint32_t)&spi->instance->DR;

    // select channel
    tx_temp_cr |= (spi->dma_tx_channel << DMA_SxCR_CHSEL_Pos);
    rx_temp_cr |= (spi->dma_rx_channel << DMA_SxCR_CHSEL_Pos);

    spi->dma_stream_tx->CR = tx_temp_cr;
    spi->dma_stream_rx->CR = rx_temp_cr;
    
    return SPI_OK;
}

spi_status_t spi_execute_transaction_dma(spi_t *spi, spi_transaction_t *tr) {
    assert(spi != NULL);
    assert(tr != NULL);

    if(spi->state != SPI_READY) {
        return SPI_BUSY;
    } 
    // init context
    spi->current_transaction = tr;
    spi->state = SPI_BUSY_TX_RX; // DMA only full duplex mode

    // disable stream for configure DMA
    spi->dma_stream_tx->CR &= ~DMA_SxCR_EN;
    while(spi->dma_stream_tx->CR & DMA_SxCR_EN);
    spi->dma_stream_rx->CR &= ~DMA_SxCR_EN;
    while(spi->dma_stream_rx->CR & DMA_SxCR_EN); 

    // reset all irq flags
    dma_clear_all_flags(spi->dma_instance, spi->dma_stream_tx);
    dma_clear_all_flags(spi->dma_instance, spi->dma_stream_rx); 

    // len of transaction
    if (tr->tx_buff != NULL && tr->rx_buff != NULL) {
        assert(tr->tx_len == tr->rx_len);
    }
    uint16_t total_len = (tr->tx_len > tr->rx_len) ? tr->tx_len : tr->rx_len;
    spi->dma_stream_tx->NDTR = total_len;
    spi->dma_stream_rx->NDTR = total_len;

    // reset memory increment
    uint32_t tx_cr = spi->dma_stream_tx->CR & ~DMA_SxCR_MINC;
    uint32_t rx_cr = spi->dma_stream_rx->CR & ~DMA_SxCR_MINC;

    // TX stream
    if(tr->tx_buff != NULL) { // if send
        spi->dma_stream_tx->M0AR = (uint32_t)tr->tx_buff; 
        tx_cr |= DMA_SxCR_MINC; // increment memory (buffer) 
    } else {  // if we only read we must to send a dummy bites
        spi->dma_stream_tx->M0AR = (uint32_t)&dma_dummy_tx;
        // MINC disable, DMA will cyclically read the same byte, 0xFF
    }
    // RX stream
    if(tr->rx_buff != NULL) { // if read
        spi->dma_stream_rx->M0AR = (uint32_t)tr->rx_buff;
        rx_cr |= DMA_SxCR_MINC; // increment memory (buffer)
    } else { // if we only send we must to read a dummy bites
        spi->dma_stream_rx->M0AR = (uint32_t)&dma_dummy_rx;
        // MINC disable, DMA will accumulate all incoming bytes into dummy_rx
    }

    // Update CR
    spi->dma_stream_tx->CR = tx_cr;
    spi->dma_stream_rx->CR = rx_cr | DMA_SxCR_TCIE; // and enable TCIE irq for RX only

    // Enable DMA streams
    spi->dma_stream_rx->CR |= DMA_SxCR_EN;
    spi->dma_stream_tx->CR |= DMA_SxCR_EN;

    // Enable errors IRQ
    spi->instance->CR2 |= SPI_CR2_ERRIE;

    // Enable DMA
    spi->instance->CR2 |= (SPI_CR2_TXDMAEN | SPI_CR2_RXDMAEN);

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

static void DMAx_IRQHandler(int stream_idx) {
    spi_t *spi = registered_dma_rx[stream_idx]; // get spi obj
    if(spi == NULL) return;
    
    // check TC flag (TCIF): NDTR = 0
    if(dma_is_tc_flag_set(spi->dma_instance, stream_idx)) {
        // reset all flags in current stream
        dma_clear_all_flags(spi->dma_instance, spi->dma_stream_rx);
        dma_clear_all_flags(spi->dma_instance, spi->dma_stream_tx);
        // disable irqs
        spi->instance->CR2 &= ~(SPI_CR2_TXDMAEN | SPI_CR2_RXDMAEN | SPI_CR2_ERRIE);
        // wait last bite
        while(spi->instance->SR & SPI_SR_BSY);

        if(spi->current_transaction->rx_buff == NULL) { //
            volatile uint32_t dummy = spi->instance->DR; //
            dummy = spi->instance->SR; // reset OVR
            (void)dummy; //
        }

        spi->state = SPI_READY; 
        if(spi->current_transaction && spi->current_transaction->callback) { 
            spi->current_transaction->callback(spi, spi->current_transaction, SPI_OK);
        }
    }
}


// DMA interrupts 
void DMA1_Stream0_IRQHandler(void) { DMAx_IRQHandler(0);}
void DMA1_Stream1_IRQHandler(void) { DMAx_IRQHandler(1);}
void DMA1_Stream2_IRQHandler(void) { DMAx_IRQHandler(2);}
void DMA1_Stream3_IRQHandler(void) { DMAx_IRQHandler(3);}
void DMA1_Stream4_IRQHandler(void) { DMAx_IRQHandler(4);}
void DMA1_Stream5_IRQHandler(void) { DMAx_IRQHandler(5);}
// void DMA1_Stream6_IRQHandler(void) { DMAx_IRQHandler(6);}               // reserved for USART2
void DMA1_Stream7_IRQHandler(void) { DMAx_IRQHandler(7);}
void DMA2_Stream0_IRQHandler(void) { DMAx_IRQHandler(8);}
// void DMA2_Stream1_IRQHandler(void) { DMAx_IRQHandler(9);}               // reserved for ADC3  
// void DMA2_Stream2_IRQHandler(void) { DMAx_IRQHandler(10); }             // reserved for ADC2
void DMA2_Stream3_IRQHandler(void) { DMAx_IRQHandler(11); }
// void DMA2_Stream4_IRQHandler(void) { DMAx_IRQHandler(12); }             // reserved for ADC1
void DMA2_Stream5_IRQHandler(void) { DMAx_IRQHandler(13); }
void DMA2_Stream6_IRQHandler(void) { DMAx_IRQHandler(14); }
void DMA2_Stream7_IRQHandler(void) { DMAx_IRQHandler(15); }
