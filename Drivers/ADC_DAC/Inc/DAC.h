#ifndef DAC_H
#define DAC_H

#include "stm32f446xx.h"
#include <stdint.h>
#include <stdbool.h>
#include <timer.h>

typedef struct {
    DAC_TypeDef *instance;          // support only DAC1
    DMA_TypeDef *dma_instance;
    DMA_Stream_TypeDef *dma_stream; 
    uint32_t dma_channel;

    uint16_t *wave_buffer;      // buffer in RAM
    uint16_t num_points;        // num of points in buffer
    
} dac_t;

void dac_init(dac_t *dac, timer_basic_t *timer);
void dac_start(dac_t *dac);

#endif
