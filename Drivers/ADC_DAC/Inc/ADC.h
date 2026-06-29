/**
 * @file ADC.h
 * @brief Bare-metal ADC (Analog-to-Digital Converter) Peripheral Library for STM32F446xx.
 * * Provides a lightweight, low-level abstraction layer for managing ADC1, ADC2, and ADC3
 * using DMA2 for automated, non-blocking data streaming into RAM buffers without CPU overhead.
 * * @author Sergei
 * @date 2026-06-29
 */

#ifndef ADC_H
#define ADC_H

#include "stm32f446xx.h"
#include <stdint.h>
#include <assert.h>

/* Forward declaration of the main ADC handle structure */
struct adc_t;
typedef struct adc_t adc_t;

/**
 * @enum adc_mode_t
 * @brief Defines the operational modes of the ADC peripheral.
 */
typedef enum {
    CONTINUOUS,   /**< Continuously converts configured channels in a loop indefinitely. */
    SCAN,         /**< Scans a sequence of multiple channels once per trigger. */
    TIME_TRIGGER  /**< Hardware timer events trigger the conversion sequence. */
} adc_mode_t;

/**
 * @struct adc_channel_config_t
 * @brief Individual hardware ADC channel configuration parameter mapping.
 */
typedef struct {
    uint8_t channel_number;                     /**< Hardware channel index (e.g., 0 for PA0, 1 for PA1). */
    uint8_t sampling_time;                      /**< Sample time selection bit pattern (defines capacitor charge cycles). */
} adc_channel_config_t;

/**
 * @brief User-defined callback function pointer type for DMA transfer complete events.
 * @param[in] adc Pointer to the active ADC handle structure invoking the callback.
 */
typedef void (*adc_callback_t)(adc_t *adc);

/**
 * @struct adc_t
 * @brief Main ADC driver handle structure holding hardware configurations and context state.
 */
typedef struct adc_t {
    ADC_TypeDef *instance;                      /**< Pointer to the hardware instance base register (ADC1, ADC2, or ADC3). */
    DMA_TypeDef *dma_instance;                  /**< Pointer to the DMA controller base register (managed automatically as DMA2). */
    DMA_Stream_TypeDef *dma_stream;            /**< Pointer to the specific allocated DMA Stream hardware registers. */
    uint32_t dma_channel;                       /**< Assigned DMA hardware channel option value. */
    adc_mode_t mode;                            /**< Operational conversion mode (Continuous, Scan, or Triggered). */
    uint16_t sample_time;                       /**< Global fallback sample time configuration value. */
    uint32_t tcif_mask;                         /**< Pre-calculated bitmask for detecting the DMA Transfer Complete Interrupt Flag. */

    adc_channel_config_t *adc_channels;         /**< Pointer to user-allocated array containing channel-specific settings. */
    uint8_t num_channels;                       /**< Number of active channels defined in the array (Maximum: 6). */

    uint16_t *data_buffer;                      /**< Pointer to destination memory buffer (RAM) for automated DMA streaming. */

    adc_callback_t callback;                    /**< Optional user callback executed inside the ISR context upon buffer completion. */
} adc_t;


/**
 * @brief Initializes the hardware ADC instance and its assigned DMA stream.
 * * Sets up clock routing, maps pin configurations to analog roles, configures sequence ordering,
 * establishes standard 12-bit data scaling with right-alignment, matches 16-bit word widths 
 * (MSIZE/PSIZE = Half-Word) to avoid clipping, and activates circular DMA routing.
 * * @note This function handles internal routing maps for ADC1/2/3 directly to DMA2.
 * @param[in,out] adc Pointer to the pre-filled driver handle configuration context.
 */
void adc_init(adc_t *adc);

/**
 * @brief Commands the hardware pipelines to activate and begins capturing samples.
 * * Maps DMA trigger linkages within the ADC core registers, establishes continuous generation requests,
 * wakes up the specific core tracking engine (ADON), engages the automated DMA data stream pipeline,
 * and issues a software start command (SWSTART) if configured for independent loops.
 * * @param[in] adc Pointer to the fully initialized driver handle structure.
 */
void adc_start(adc_t *adc);

/**
 * @brief Global processing routine designed to service hardware DMA Transfer Complete interrupts.
 * * This routine handles low-level register tracking logic for DMA status streams, resets pending
 * hardware transfer-complete flags inside LIFCR/HIFCR registers, and forwards tracking execution 
 * context smoothly to the custom user callback block.
 * * @note Must be manually included inside corresponding DMA2 vector lines (e.g., `DMA2_Stream4_IRQHandler`).
 * @param[in] idx Registered tracking index mapping to active units (0 for ADC1, 1 for ADC2, 2 for ADC3).
 */
void ADC_DMAx_IRQHandler(uint8_t idx);

#endif /* ADC_H */