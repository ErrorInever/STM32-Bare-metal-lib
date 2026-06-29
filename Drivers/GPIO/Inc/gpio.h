/**
 * @file gpio.h
 * @brief Bare-metal GPIO (General Purpose Input/Output) & EXTI Controller Library for STM32F446xx.
 *
 * Provides a highly optimized, low-level abstraction layer for basic pin configuration,
 * high-speed atomic atomic digital I/O using BSRR/IDR, and external interrupt (EXTI) line mapping
 * paired with individual runtime callbacks.
 *
 * @author ErrorInever
 * @date 2026-06-29
 */

#ifndef GPIO_H_
#define GPIO_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <assert.h>
#include <stdint.h>
#include <stdbool.h>
#include "stm32f446xx.h"

/** * @struct gpio_t
 * @brief GPIO Pin Descriptor.
 *
 * Encapsulates a single hardware pin reference on a specific port.
 */
typedef struct {
    GPIO_TypeDef *port;   /**< Base register pointer of the target GPIO peripheral (GPIOA, GPIOB, etc.). */
    uint16_t pin;         /**< Pin index number (Valid range: 0 to 15). */
} gpio_t;

/** * @enum gpio_mode_t
 * @brief Standard hardware pin operating modes.
 */
typedef enum {
    GPIO_MODE_INPUT_t  = 0x00, /**< Digital Input Mode. */
    GPIO_MODE_OUTPUT_t = 0x01, /**< Digital Push-Pull / Open-Drain Output Mode. */
    GPIO_MODE_AF_t     = 0x02, /**< Alternate Function Multiplexer Mode (e.g., UART, SPI, I2C). */
    GPIO_MODE_ANALOG_t = 0x03  /**< Analog Input/Output Mode for ADC or DAC isolation. */
} gpio_mode_t;

/** * @enum gpio_pull_t
 * @brief Internal pull-up or pull-down resistor electrical configurations.
 */
typedef enum {
    GPIO_PULL_NONE_t = 0x00, /**< No internal resistor connection (Floating). */
    GPIO_PULL_UP_t   = 0x01, /**< Internal weak pull-up resistor engaged. */
    GPIO_PULL_DOWN_t = 0x02  /**< Internal weak pull-down resistor engaged. */
} gpio_pull_t;

/** * @enum gpio_speed_t
 * @brief Output driver slew rate / frequency response controls.
 */
typedef enum {
    GPIO_SPEED_LOW_t      = 0x00, /**< Low speed (Max 2 MHz - minimal electrical noise). */
    GPIO_SPEED_MEDIUM_t   = 0x01, /**< Medium speed (Max 12.5 MHz to 50 MHz). */
    GPIO_SPEED_HIGH_t     = 0x02, /**< Fast speed (Max 25 MHz to 100 MHz). */
    GPIO_SPEED_VERY_HIGH_t = 0x03 /**< High speed (Max 50 MHz to 200 MHz - critical for high-speed buses). */
} gpio_speed_t;

/** * @enum gpio_otype_t
 * @brief Output driver electrical configuration types.
 */
typedef enum {
    GPIO_OTYPE_PP_t = 0x00, /**< Push-Pull output stage (Actively drives both High and Low levels). */
    GPIO_OTYPE_OD_t = 0x01  /**< Open-Drain output stage (Requires external pull-up for High level). */
} gpio_otype_t;

/** * @enum edge_type_t
 * @brief Edge trigger detection conditions for EXTI external hardware interrupts.
 */
typedef enum {
    RISING_EDGE_t,  /**< Interrupt triggers exclusively on a Low-to-High signal transition. */
    FALLING_EDGE_t, /**< Interrupt triggers exclusively on a High-to-Low signal transition. */
    RISING_FALLING_t /**< Interrupt triggers on both signal state transitions. */
} edge_type_t;

/**
 * @brief User callback function pointer definition for handling EXTI interrupt triggers.
 * @param[in] gpio Pointer to the static or registered `gpio_t` descriptor causing the interrupt.
 */
typedef void (*gpio_callback_t)(const gpio_t *gpio);


/**
 * @brief Registers a custom runtime interrupt handler callback routine for an EXTI line.
 * * @note The internal lookup table allocates callbacks globally based on the pin number (0-15).
 * Sharing a pin index across different ports (e.g., PA0 and PB0) will overwrite the entry.
 *
 * @param[in] gpio     Pointer to the target GPIO descriptor context.
 * @param[in] callback User-defined function pointer to be executed within the ISR.
 */
void gpio_set_callback(const gpio_t *gpio, gpio_callback_t callback);

/**
 * @brief Performs complete initialization of a GPIO pin according to specified parameters.
 * * Automatically enables the appropriate clock gating in the RCC block, sets basic modes,
 * output types, operational speeds, and internal pull configurations inside the peripheral registers.
 *
 * @param[in] gpio   Pointer to the specific pin descriptor to initialize.
 * @param[in] mode   Desired operating mode (Input, Output, AF, Analog).
 * @param[in] pull   Internal resistor mapping (None, Pull-Up, Pull-Down).
 * @param[in] speed  Output signal transition rate performance level.
 * @param[in] otype  Output driver hardware configuration (Push-Pull or Open-Drain).
 */
void gpio_init(const gpio_t *gpio, gpio_mode_t mode, gpio_pull_t pull, gpio_speed_t speed, gpio_otype_t otype);

/**
 * @brief Configures the alternate function multiplexer routing for a given pin.
 * * Handles the complex bit shifting logic to map the 4-bit alternate function code
 * to either the Low (`AFR[0]`) or High (`AFR[1]`) multiplexing register.
 *
 * @param[in] gpio   Pointer to the specific pin descriptor.
 * @param[in] af_num Alternate function index number to apply (0 to 15).
 */
void gpio_set_af(const gpio_t *gpio, uint8_t af_num);

/**
 * @brief Enters low-level setup routines to connect and activate external edge hardware interrupts.
 * * Connects the physical pin through the `SYSCFG->EXTICR` multiplexer matrix to the designated 
 * EXTI core line, configs the trigger registers (`RTSR`/`FTSR`), unmasks the interrupt line, 
 * and enables the appropriate global vector address line within the core NVIC structure.
 *
 * @param[in] gpio Pointer to the pin structure acting as the trigger source.
 * @param[in] edge Edge detection condition requirement flag (Rising, Falling, or Both).
 */
void gpio_enable_irq(const gpio_t *gpio, edge_type_t edge);


/** * @brief Writes a digital logic state to a GPIO output pin.
 * * Utilizes safe, atomic register access via the peripheral's Bit Set/Reset Register (`BSRR`).
 * This approach bypasses standard Read-Modify-Write hazards without locking core routines.
 * * @param[in] gpio  Pointer to the active pin descriptor context.
 * @param[in] state Logical output level selection (0 = LOW, non-zero = HIGH). 
 */
static inline void gpio_write(const gpio_t *gpio, uint8_t state) {
    if(state) {
        gpio->port->BSRR = (1U << gpio->pin);
    } else {
        gpio->port->BSRR = (1U << (gpio->pin + 16U));
    }
}

/** * @brief Reads the actual live digital logic level present on a physical pin.
 * * Captures current states directly by reading the peripheral Input Data Register (`IDR`).
 * * @param[in] gpio Pointer to the target pin descriptor context.
 * @return Current digital logic state (0 if level is LOW, 1 if level is HIGH). 
 */
static inline uint8_t gpio_read(const gpio_t *gpio) {
    return (gpio->port->IDR & (1U << gpio->pin)) ? 1 : 0;
}

/** * @brief Atomically forces a GPIO output pin to a HIGH state.
 * * Operates via a single-cycle write to the `BSRR` register.
 * * @param[in] gpio Pointer to the target pin descriptor context.
 */
static inline void gpio_set(const gpio_t *gpio) {
    gpio->port->BSRR = (1U << gpio->pin);
}

/** * @brief Atomically forces a GPIO output pin to a LOW state.
 * * Operates via a single-cycle write to the upper 16-bits of the `BSRR` register.
 * * @param[in] gpio Pointer to the target pin descriptor context.
 */
static inline void gpio_reset(const gpio_t *gpio) {
    gpio->port->BSRR = (1U << (gpio->pin + 16U));
}

/** * @brief Inverts the current digital logic output state of a pin.
 * * Accesses the Output Data Register (`ODR`) to toggle the targeted bitmask value.
 * * @param[in] gpio Pointer to the target pin descriptor context.
 */
static inline void gpio_toggle(const gpio_t *gpio) {
    gpio->port->ODR ^= (1U << gpio->pin);
}

#ifdef __cplusplus
}
#endif

#endif /* GPIO_H_ */