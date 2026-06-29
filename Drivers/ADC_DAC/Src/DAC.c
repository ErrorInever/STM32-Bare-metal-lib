#include "DAC.h"
#include "stm32f446xx.h"
#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <timer.h>


void dac_init(dac_t *dac, timer_basic_t *timer) {
    assert(dac != NULL);
    assert(dac->instance == DAC1);

    // RCC DMA and DAC
    RCC->AHB1ENR |= RCC_AHB1ENR_DMA1EN;
    RCC->APB1ENR |= RCC_APB1ENR_DACEN;
    (void)RCC->APB1ENR;

    // enable IRQ in NVIC 
    //NVIC_EnableIRQ(DMA1_Stream5_IRQn);

    // init param DAC1 (DAC channel 1 - PA4)
    dac->instance = DAC1;
    dac->dma_instance = DMA1;
    dac->dma_stream = DMA1_Stream5;
    dac->dma_channel = 7U;

    // disable stream
    dac->dma_stream->CR &= ~DMA_SxCR_EN;
    while(dac->dma_stream->CR & DMA_SxCR_EN);

    // reset DMA settings
    DMA1->LIFCR = 0xFFFFFFFFU;
    DMA1->HIFCR = 0xFFFFFFFFU;

    // setup addresses
    // (Data Holding Register, 12-bit, Right-aligned, Channel 1
    dac->dma_stream->PAR = (uint32_t)&dac->instance->DHR12R1;
    dac->dma_stream->M0AR = (uint32_t)dac->wave_buffer;         // wave of signal
    dac->dma_stream->NDTR = dac->num_points;                    // num of points

    // DMA config
    uint32_t cr = 0;
    cr |= (dac->dma_channel << DMA_SxCR_CHSEL_Pos);   // select channel (7)
    cr |= DMA_SxCR_MINC;                                        // increment memory
    cr &= ~DMA_SxCR_PINC;                                       // disable peref increment
    cr |= (1U << DMA_SxCR_DIR_Pos);                             // direction M2P 
    cr |= DMA_SxCR_CIRC;                                        // mode Circular
    cr |= (1U << DMA_SxCR_MSIZE_Pos);                           // size memory 16 bit (half-word)
    cr |= (1U << DMA_SxCR_PSIZE_Pos);                           // size periph 16 bit (half-word)
    dac->dma_stream->CR = cr;

    // Configure timer
    timer->instance->CR2 &= ~TIM_CR2_MMS;
    timer->instance->CR2 |= (2U << TIM_CR2_MMS_Pos); // 010: Update event is used as trigger output

    // DAC config
    // reset CR
    dac->instance->CR &= ~0xFFFFU;
    dac->instance->CR |= DAC_CR_TEN1;   // enable trigger TEN1 = 1

    // enable DMA in DAC
    dac->instance->CR |= DAC_CR_DMAEN1;
    // enable DAC module
    dac->instance->CR |= DAC_CR_EN1;
    // enable timer
    timer->instance->CR1 |= TIM_CR1_CEN;
}

void dac_start(dac_t *dac) {
    // enable stream
    dac->dma_stream->CR |= DMA_SxCR_EN;
}