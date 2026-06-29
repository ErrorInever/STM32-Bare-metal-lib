/**
 * @file i2c.h
 * @brief Non-blocking, Interrupt-driven I2C Master Library with FSM for STM32F446xx.
 *
 * This library provides a bare-metal abstraction for standard and fast-mode I2C master communications.
 * It utilizes a Finite State Machine (FSM) within the Event and Error ISR handlers to execute
 * non-blocking asynchronous data transfers, featuring automated recovery and hardware retry logic.
 *
 * @author ErrorInever
 * @date 2026-06-29
 */

#ifndef I2C_H
#define I2C_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "stm32f446xx.h"

/** * @enum i2c_status_t
 * @brief I2C Transaction and Interface operational results.
 */
typedef enum {
    I2C_OK,               /**< Transaction completed successfully. */
    I2C_TIMEOUT,          /**< Hardware timeout or bus lock detected. */
    I2C_NACK,             /**< Acknowledge failure received from the slave device (Address or Data). */
    I2C_BUS_ERROR,        /**< Misplaced Start or Stop condition detected on the bus. */
    I2C_ARBITRATION_LOST, /**< Master lost bus control priority to another master device. */
    I2C_INVALID_ARG,      /**< Out of bounds parameters passed to initialization or execution functions. */
    I2C_IS_BUSY,          /**< Peripheral or bus is currently locked by an active transaction. */
    I2C_QUEUE_FULL        /**< Internal transaction dispatch buffer capacity reached. */
} i2c_status_t;

/** * @enum i2c_state_t
 * @brief Finite State Machine (FSM) positions for non-blocking interrupt tracking.
 */
typedef enum {
    I2C_IDLE,             /**< Peripheral is relaxed; no active communication on the bus. */
    I2C_START,            /**< Start condition generated; waiting for SB (Start Bit) hardware flag. */
    I2C_ADDR,             /**< Slave Address byte transmitted; waiting for ADDR match clearance flag. */
    I2C_TX,               /**< Asynchronous data transmission loop actively flushing data from RAM. */
    I2C_RX,               /**< Asynchronous data reception loop actively capturing incoming bytes. */
    I2C_STOP,             /**< Stop condition commanded; waiting for bus release verification. */
    I2C_DONE,             /**< FSM successfully reached its valid terminal condition. */
    I2C_ERROR             /**< Exception encountered; running hardware recovery routines. */
} i2c_state_t;

/** * @enum i2c_mode_t
 * @brief SCL clock frequency operational standards.
 */
typedef enum {
    I2C_SM_100KHZ,        /**< Standard Mode communication speed up to 100 kHz. */
    I2C_FM_400KHZ         /**< Fast Mode communication speed up to 400 kHz. */
} i2c_mode_t;

/** * @struct i2c_transaction_t
 * @brief Communication Context Profile containing memory addresses and control switches.
 */
typedef struct {
    uint16_t addr;          /**< 7-bit Target Slave Address. @note Pass a raw 7-bit value; do not pre-shift left. */
    uint8_t *tx_buff;       /**< Pointer to the source data buffer in RAM for transmission. */
    size_t tx_len;          /**< Total number of data bytes allocated for transmission. */
    uint8_t *rx_buff;       /**< Pointer to the destination buffer in RAM for storing received bytes. */
    size_t rx_len;          /**< Total number of data bytes expected to be read. */
    bool repeated_start;    /**< True: Issue Repeated Start instead of a Stop condition between TX and RX phases. */
    uint8_t max_retries;    /**< Max number of automatic transmission restarts if the hardware receives a NACK. */
} i2c_transaction_t;

/**
 * @brief User Callback routine signature for background reporting of completed operations.
 * @param[in] status Final operational verdict of the transaction pipeline (e.g., I2C_OK, I2C_NACK).
 */
typedef void (*i2c_callback_t)(i2c_status_t status);

/** * @struct i2c_t
 * @brief Comprehensive I2C Controller Object handle.
 *
 * Manages the register interface base, volatile state machine trackers, internal backup references
 * for automatic retry sequences, and active runtime notification links.
 */
typedef struct {
    I2C_TypeDef *bus;                    /**< Core peripheral base pointer mapping (I2C1, I2C2, or I2C3). */
    volatile i2c_state_t state;          /**< Volatile FSM tracking state evaluated within ISRs. */
    volatile i2c_status_t status;        /**< Asynchronous transaction result status latch. */
    i2c_transaction_t ctx;               /**< Internal clone layout of the active operational context. */
    i2c_transaction_t *current_ctx;      /**< Persistent master handle reference pointer used for retry setups. */
    uint8_t retry_cnt;                   /**< Current iteration tally of triggered recovery retries. */
    i2c_callback_t callback;             /**< Registered user event callback triggered on completion or failure. */
} i2c_t;


/**
 * @brief Initializes the specified I2C peripheral bus with targeted clock configurations.
 * * Handles clock gating activation via RCC, soft resets the peripheral, computes and configures
 * the clock control register (`CCR`), maximum rise time (`TRISE`), and activates internal interrupts.
 *
 * @param[in,out] i2c         Pointer to the target I2C instance controller structure.
 * @param[in]     pclk1_mhz   Live APB1 bus clock speed value in MHz (Valid range: 2 to 50 MHz).
 * @param[in]     mode        Desired bus timing mode profile (Standard or Fast Mode).
 * @return Operation status verdict. Returns `I2C_OK` upon complete success, or `I2C_INVALID_ARG`.
 */
i2c_status_t i2c_init(i2c_t *i2c, uint32_t pclk1_mhz, i2c_mode_t mode);

/**
 * @brief Configures a completion handler callback linked to a specific I2C module instance.
 *
 * @param[in,out] i2c      Pointer to the active I2C controller structure.
 * @param[in]     callback Target function pointer designed to handle transaction completion status.
 */
void i2c_set_callback(i2c_t *i2c, i2c_callback_t callback);

/**
 * @brief Enters an asynchronous, non-blocking pipeline execution of an I2C transaction request.
 * * This function verifies bus availability, registers the transaction context, enables peripheral
 * interrupt event/error masks, and issues the hardware START condition bit. The remainder of the
 * transaction is driven entirely by the I2C Event/Error ISRs.
 * * @note If a master TX/RX multi-stage sequence utilizes `repeated_start`, the FSM avoids generating
 * a STOP bit between phases and seamlessly triggers a secondary START condition instead.
 *
 * @param[in,out] i2c         Pointer to the active I2C controller structure.
 * @param[in]     transaction Pointer to the configured payload tracking parameter block.
 * @return Verification check status. Returns `I2C_OK` if accepted into the pipeline, or `I2C_IS_BUSY`.
 */
i2c_status_t i2c_transfer_it(i2c_t *i2c, i2c_transaction_t *transaction);

/**
 * @brief Top-level execution driver routine serving Core Event Interrupt Service Routines.
 * * This function must be placed inside the active startup vector table bindings (`I2C1_EV_IRQHandler`, etc.).
 * It processes the hardware event state transitions (SB, ADDR, TXE, RXNE, BTF) and updates the internal FSM.
 *
 * @param[in,out] i2c Pointer to the active I2C controller structure processing hardware event ticks.
 */
void I2Cx_EV_IRQ_execute(i2c_t *i2c);

/**
 * @brief Top-level execution driver routine serving Core Error Interrupt Service Routines.
 * * This function must be placed inside the active startup vector table bindings (`I2C1_ER_IRQHandler`, etc.).
 * It monitors hardware error flags (AF/NACK, BERR, ARLO) and manages retry logic or triggers error states.
 *
 * @param[in,out] i2c Pointer to the active I2C controller structure processing error triggers.
 */
void I2Cx_ER_IRQ_execute(i2c_t *i2c);

#ifdef __cplusplus
}
#endif

#endif /* I2C_H */