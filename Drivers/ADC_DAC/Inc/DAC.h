/**
 * @file DAC.h
 * @brief Bare-metal DAC (Digital-to-Analog Converter) Peripheral Library for STM32F446xx.
 *
 * Provides a lightweight, low-level abstraction layer for managing DAC1 (Channel 1, Pin PA4)
 * using DMA1 and a basic timer trigger for automated, automated hardware wave generation 
 * without CPU overhead.
 *
 * @author ErrorInever
 * @date 2026-06-29
 */

#ifndef DAC_H
#define DAC_H

#include "stm32f446xx.h"
#include "timer.h"
#include <stdint.h>
#include <stdbool.h>

/**
 * @struct dac_t
 * @brief Main DAC driver handle structure holding hardware configurations and buffer state.
 * * @note This implementation specifically targets DAC1 Channel 1 mapped to GPIO Pin PA4.
 */
typedef struct {
    DAC_TypeDef *instance;          /**< Pointer to the hardware instance base register (Supports DAC1). */
    DMA_TypeDef *dma_instance;      /**< Pointer to the DMA controller base register (Managed automatically as DMA1). */
    DMA_Stream_TypeDef *dma_stream; /**< Pointer to the specific allocated DMA Stream hardware registers (DMA1_Stream5). */
    uint32_t dma_channel;           /**< Assigned DMA hardware channel option value (Channel 7). */

    uint16_t *wave_buffer;          /**< Pointer to source memory buffer (RAM) containing digital wave samples. */
    uint16_t num_points;            /**< Total number of signal points/samples allocated inside the wave buffer. */
} dac_t;

/**
 * @brief Initializes the hardware DAC instance, its assigned DMA stream, and trigger timer.
 *
 * This function performs low-level configuration of the DAC-DMA pipeline:
 * - Enables clock routing for DMA1 and DAC1 peripherals.
 * - Auto-configures internal handle mappings for DAC1, DMA1 Stream 5, and Channel 7.
 * - Resets and sets up DMA data stream dimensions (PAR mapped to DHR12R1, M0AR mapped to wave buffer).
 * - Establishes circular memory access structure (`CIRC`, `MINC`) with 16-bit word widths (`MSIZE`/`PSIZE` = Half-Word).
 * - Adjusts the provided basic timer's Master Mode Selection (`MMS = 010`) to output Update Events as TRGO.
 * - Pairs the DAC output trigger event line to align with the timer tracking interval (`TEN1 = 1`).
 *
 * @param[in,out] dac   Pointer to the driver handle structure to be filled and initialized.
 * @param[in,out] timer Pointer to the pre-configured basic timer instance (e.g., TIM6) driving the update rate.
 */
void dac_init(dac_t *dac, timer_basic_t *timer);

/**
 * @brief Commands the hardware pipelines to activate and begins generating the analog wave.
 *
 * Enables the designated hardware DMA stream to begin shifting values out of RAM, 
 * and flips the activation bit inside the DAC control register to route the analog voltage
 * continuously to physical pin PA4.
 *
 * @param[in] dac Pointer to the fully initialized DAC driver handle structure.
 */
void dac_start(dac_t *dac);

#endif /* DAC_H */