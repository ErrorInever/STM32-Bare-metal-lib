#include "ADC.h"
#include "stm32f446xx.h"
#include <assert.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>


#define NUM_ADC_CNT 3   // number of ADC interface count. current version have 3

static adc_t *adc_handler[NUM_ADC_CNT] = {NULL};

static void adc_clock_enable(const adc_t *adc) {
    assert(adc != NULL);
    if(adc->instance == ADC1) { RCC->APB2ENR |= RCC_APB2ENR_ADC1EN; }
    else if(adc->instance == ADC2) { RCC->APB2ENR |= RCC_APB2ENR_ADC2EN; }
    else if(adc->instance == ADC3) { RCC->APB2ENR |= RCC_APB2ENR_ADC3EN; }
}

static void setup_stream(adc_t *adc) {
    assert(adc != NULL);

    adc->dma_instance = DMA2; 

    if(adc->instance == ADC1) { 
        adc->dma_stream = DMA2_Stream4;
        adc->dma_channel = 0U;
        adc->tcif_mask = DMA_HISR_TCIF4; // Маска для HISR
    } else if(adc->instance == ADC2) {
        adc->dma_stream = DMA2_Stream2;
        adc->dma_channel = 0U;
        adc->tcif_mask = DMA_LISR_TCIF2; // Маска для LISR
    } else if(adc->instance == ADC3) {
        adc->dma_stream = DMA2_Stream1;
        adc->dma_channel = 0U;
        adc->tcif_mask = DMA_LISR_TCIF1; // Маска для LISR
    } 
}

static void registred_adc(adc_t *adc) {
    assert(adc != NULL);
    if(adc->instance == ADC1) { 
        adc_handler[0] = adc;
    } else if(adc->instance == ADC2) {
        adc_handler[1] = adc;
    } else if(adc->instance == ADC3) {
        adc_handler[2] = adc;
    } 
}

static void enable_irq_NVIC(const DMA_Stream_TypeDef *stream) {
    assert(stream != NULL);
    if(stream == DMA2_Stream1) { NVIC_EnableIRQ(DMA2_Stream1_IRQn); }
    else if(stream == DMA2_Stream2) { NVIC_EnableIRQ(DMA2_Stream2_IRQn); }
    else if(stream == DMA2_Stream4) { NVIC_EnableIRQ(DMA2_Stream4_IRQn); }
}

static void adc_configure_queue_simple(adc_t *adc, uint8_t global_sampling_time) {
    // If channels > 1 enable Scan MODE
    if (adc->num_channels > 1) {
        adc->instance->CR1 |= ADC_CR1_SCAN;
    }

    // Setup length queue in SQR1 
    adc->instance->SQR1 &= ~ADC_SQR1_L;
    adc->instance->SQR1 |= ((adc->num_channels - 1) << ADC_SQR1_L_Pos);

    // Reset channels
    adc->instance->SQR3 = 0;

    for (uint8_t i = 0; i < adc->num_channels; i++) {
        uint8_t channel_num = adc->adc_channels[i].channel_number;
        adc->instance->SQR3 |= (channel_num << (i * 5));
        // setup sample time
        if (channel_num <= 9) {
            adc->instance->SMPR2 &= ~(7U << (channel_num * 3));
            adc->instance->SMPR2 |= (global_sampling_time << (channel_num * 3));
        }
    }
}

void adc_init(adc_t *adc, adc_mode_t mode) {
    assert(adc != NULL);
    assert(adc->instance == ADC1 || adc->instance == ADC2 || adc->instance == ADC3);

    // enable RCC ADC & DMA
    adc_clock_enable(adc);
    RCC->AHB1ENR |= RCC_AHB1ENR_DMA2EN;
    (void)RCC->AHB1ENR;

    // setup DMA stream and channel
    setup_stream(adc);
    // enable NVIC interrupts
    enable_irq_NVIC(adc->dma_stream);
    // registred adc
    registred_adc(adc);
    
    // init param
    adc->mode = mode;
    adc->sample_time = 3U; // 56 cycles

    // disable stream for configure DMA
    adc->dma_stream->CR &= ~DMA_SxCR_EN;
    while(adc->dma_stream->CR & DMA_SxCR_EN);
    
    // reset DMA settings
    DMA2->LIFCR = 0xFFFFFFFFU;
    DMA2->HIFCR = 0xFFFFFFFFU;
    
    // setup addresses
    adc->dma_stream->PAR = (uint32_t)&adc->instance->DR;        // from DR
    adc->dma_stream->M0AR = (uint32_t)adc->data_buffer;         // to buffer
    adc->dma_stream->NDTR = adc->num_channels;

    // DMA configure
    uint32_t cr = 0;
    cr |= ((uint32_t)adc->dma_channel << DMA_SxCR_CHSEL_Pos);   // select channel
    cr |= DMA_SxCR_MINC;                                        // increment memory
    cr &= ~DMA_SxCR_PINC;                                       // disable peref increment
    cr |= 0U;                                                   // direction P2M 
    cr |= DMA_SxCR_CIRC;                                        // mode Circular
    cr |= DMA_SxCR_TCIE;                                        // enable interrupt Transmit complete
    cr |= DMA_SxCR_TEIE;                                        // enable interrupt Transport error
    adc->dma_stream->CR = cr;

    // ADC configure
    // setup freq (max 18Mhz)
    ADC->CCR &= ~ADC_CCR_ADCPRE; // reset prescaler
    ADC->CCR |= (2 << ADC_CCR_ADCPRE_Pos); // div 4
    // setup resolution
    adc->instance->CR1 &= ~ADC_CR1_RES; // max resolution 12 bit (00)

    // allow innner channels (for TEST)
    ADC->CCR |= ADC_CCR_TSVREFE;

    // setup queue
    adc_configure_queue_simple(adc, adc->sample_time);

    if(adc->mode == CONTINUOUS) {
        adc->instance->CR2 |= ADC_CR2_CONT;
    } 
    else if(adc->mode == TIME_TRIGGER) {
        // TODO: TIM (EXTEN/EXTSEL)
    }

    // Allow DMA request generation for ADC
    // Allow continuous generation of requests (DDS = DMA Disable Selection)
    adc->instance->CR2 |= ADC_CR2_DMA | ADC_CR2_DDS;
    // Enable ADC
    adc->instance->CR2 |= ADC_CR2_ADON;
    // Enable stream DMA
    adc->dma_stream->CR |= DMA_SxCR_EN;
    // Start
    if(adc->mode == CONTINUOUS || adc->mode == SCAN) {
        adc->instance->CR2 |= ADC_CR2_SWSTART;
    }
}

void ADC_DMAx_IRQHandler(uint8_t idx) {
    adc_t *adc = adc_handler[idx]; 
    if(adc == NULL) return;

    // stream 1-2 in LIFCR/LISR
    if(adc->dma_stream == DMA2_Stream1 || adc->dma_stream == DMA2_Stream2) {
        if(adc->dma_instance->LISR & adc->tcif_mask) {
            adc->dma_instance->LIFCR = adc->tcif_mask;
            if(adc->callback != NULL) {
                adc->callback(adc);
            }
        }
    }
    // stream 4 in HIFCR/HISR
    else if(adc->dma_stream == DMA2_Stream4) {
        if(adc->dma_instance->HISR & adc->tcif_mask) {
            adc->dma_instance->HIFCR = adc->tcif_mask;
            
            if(adc->callback != NULL) {
                adc->callback(adc);
            }
        }

    }
}


void DMA2_Stream1_IRQHandler(void) { ADC_DMAx_IRQHandler(2); }      // ADC3  
void DMA2_Stream2_IRQHandler(void) { ADC_DMAx_IRQHandler(1);  }     // ADC2
void DMA2_Stream4_IRQHandler(void) { ADC_DMAx_IRQHandler(0);  }     // ADC1
