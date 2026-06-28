#ifndef ADC_H
#define ADC_H

#include "stm32f446xx.h"
#include <stdint.h>
#include <assert.h>

struct adc_t;
typedef struct adc_t adc_t;

typedef enum {
    CONTINUOUS,
    SCAN,
    TIME_TRIGGER
} adc_mode_t;

// ADC channel desctiption (MAX 6 channels)
typedef struct {
    uint8_t channel_number;                     // channel number (ADC_CHANNEL_0 etc)
    uint8_t sampling_time;                      // sample time
} adc_channel_config_t;

// user callback
typedef void (*adc_callback_t)(adc_t *adc);

// ADC
typedef struct adc_t {
    ADC_TypeDef *instance;                      // ADC1, ADC2, ADC3
    DMA_TypeDef *dma_instance;                  // DMA1 or DMA2
    DMA_Stream_TypeDef *dma_stream;  
    uint32_t dma_channel;
    adc_mode_t mode;
    uint16_t sample_time;
    uint32_t tcif_mask;

    adc_channel_config_t *adc_channels;         // array channels
    uint8_t num_channels;                       // count channels (MAX 6)

    uint16_t *data_buffer;                      // data array

    adc_callback_t callback;

} adc_t;

void adc_init(adc_t *adc, adc_mode_t mode);


#endif
