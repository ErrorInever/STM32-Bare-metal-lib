#ifndef GPIO_H_
#define GPIO_H_

#ifdef __cplusplus
extern "C" {
#endif


#include <assert.h>
#include <stdint.h>
#include <stdbool.h>
#include "stm32f446xx.h"


/** 
* @brief GPIO descriptor. 
* 
* Represents a single GPIO pin. 
*/
typedef struct {
    GPIO_TypeDef *port;   ///< GPIO peripheral (GPIOA, GPIOB, ...)
    uint16_t pin;         ///< Pin number (0–15)
} gpio_t;

/** 
* @brief GPIO operating mode.
 */
typedef enum {
    GPIO_MODE_INPUT_t  = 0x00,
    GPIO_MODE_OUTPUT_t = 0x01,
    GPIO_MODE_AF_t     = 0x02,
    GPIO_MODE_ANALOG_t = 0x03
} gpio_mode_t;

/** 
* @brief Internal pull resistor configuration. 
*/
typedef enum {
    GPIO_PULL_NONE_t = 0x00,
    GPIO_PULL_UP_t   = 0x01,
    GPIO_PULL_DOWN_t = 0x02
} gpio_pull_t;

/** 
* @brief Output driver type. 
*/
typedef enum {
    GPIO_OTYPE_PP_t = 0x00,  // Push-pull
    GPIO_OTYPE_OD_t = 0x01   // Open-drain
} gpio_otype_t;

/** 
* @brief GPIO output speed. 
*/
typedef enum {
    GPIO_SPEED_LOW_t       = 0x00,
    GPIO_SPEED_MEDIUM_t    = 0x01,
    GPIO_SPEED_HIGH_t      = 0x02,
    GPIO_SPEED_VERY_HIGH_t = 0x03
} gpio_speed_t;

/** 
* @brief Interrupt trigger edge. 
*/
typedef enum {
    EDGE_RISING_t,
    EDGE_FALLING_t,
    EDGE_BOTH_t
} edge_type_t;

/** 
* @brief GPIO interrupt callback function. 
* 
* @param gpio Pointer to GPIO object that generated interrupt. 
*/
typedef void (*gpio_callback_t)(const gpio_t *gpio);


/** 
* @brief Initialize GPIO with full configuration. 
* 
* Enables GPIO peripheral clock and configures: 
* - pin mode 
* - pull resistors 
* - output type 
* - output speed 
* 
* @param gpio GPIO object. 
* @param mode Pin operating mode. 
* @param pull Pull resistor configuration. 
* @param otype Output driver type. 
* @param speed Output speed. 
*/
void gpio_init(const gpio_t *gpio, gpio_mode_t mode, gpio_pull_t pull, 
    gpio_otype_t otype, gpio_speed_t speed);


/** 
* @brief Configure GPIO as input. 
* 
* Simplified wrapper around gpio_init(). 
* 
* @param gpio GPIO object. 
* @param pull Pull resistor configuration. 
*/
void gpio_init_input(const gpio_t *gpio, gpio_pull_t pull);


/** 
* @brief Configure GPIO as output. 
* 
* Uses medium output speed by default. 
* 
* @param gpio GPIO object. 
* @param push_pull 
*       true → Push-pull output 
*       false → Open-drain output 
*/
void gpio_init_output(const gpio_t *gpio, bool push_pull);


/** 
* @brief Configure alternate function for a pin. 
* 
* Pin must already be configured in AF mode. 
* 
* @param gpio GPIO object. 
* @param af_num Alternate function number (0–15). 
*/
void gpio_set_alternate_function(const gpio_t *gpio, uint8_t af_num);


/** 
* @brief Enable EXTI interrupt for GPIO. 
* 
* Configures: 
* - SYSCFG EXTI mapping 
* - EXTI trigger 
* - NVIC interrupt 
* 
* @param gpio GPIO object. 
* @param edge Interrupt trigger type. 
*/
void gpio_enable_irq(const gpio_t *gpio, edge_type_t edge);


/** 
* @brief Register interrupt callback. 
* 
* Only one callback per EXTI line is supported. 
* 
* @param gpio GPIO object. 
* @param callback Callback function. 
*/
void gpio_set_callback(const gpio_t *gpio, gpio_callback_t callback);



/** 
* @brief Write logical state to GPIO output. 
* 
* Uses atomic BSRR register access. 
* 
* @param gpio GPIO object. 
* @param state 0 = LOW, non-zero = HIGH. 
*/
static inline void gpio_write(const gpio_t *gpio, uint8_t state) {
    if(state) {
        gpio->port->BSRR = (1U << gpio->pin);
    } else {
        gpio->port->BSRR = (1U << (gpio->pin + 16U));
    }
}


/** 
* @brief Read current logic level. 
* 
* Reads actual pin state from IDR register. 
* 
* @param gpio GPIO object. 
* 
* @return Current logic level. 
*/
static inline uint8_t gpio_read(const gpio_t *gpio) {
    return (gpio->port->IDR & (1U << gpio->pin)) ? 1 : 0;
}


/** 
* @brief Set output to HIGH. 
* 
* Atomic operation using BSRR. 
* 
* @param gpio GPIO object. 
*/
static inline void gpio_set(const gpio_t *gpio) {
    gpio->port->BSRR = (1U << gpio->pin);
} 


/** 
* @brief Set output to LOW. 
* 
* Atomic operation using BSRR. 
* 
* @param gpio GPIO object. 
*/
static inline void gpio_reset(const gpio_t *gpio) {
    gpio->port->BSRR = (1U << (gpio->pin + 16U));
}


/** 
* @brief Toggle output state. 
* 
* Uses ODR register modification. 
* 
* @warning Operation is not atomic. 
* 
* @param gpio GPIO object.
*/
static inline void gpio_toggle(const gpio_t *gpio) {
    gpio->port->ODR ^= (1U << gpio->pin);
}

#ifdef __cplusplus
}
#endif

#endif
