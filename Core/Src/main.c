
#include "main.h"
#include "cmsis_gcc.h"
#include "gpio.h"
#include "spi.h"
#include "stm32_hal_legacy.h"
#include "stm32f446xx.h"
#include "stm32f4xx_hal_gpio.h"
#include "systick.h"
#include "timer.h"
#include <string.h>
#include <usart.h>
#include <stdint.h>
#include <i2c.h>
#include <spi.h>
#include <stdio.h>

/*
--------SPI PINS--------
VCC     5V
GND
SCK     PA5
MISO    PA6
MOSI    PA7
CS      PB6
------------------------
*/

#define CS_HIGH GPIO_BSRR_BS6   // set
#define CS_LOW GPIO_BSRR_BR6   // reset


void SystemClock_Config(void);


// user callback

void sd_callback(spi_t *spi, volatile spi_transaction_t *tr, spi_status_t status_tr) {
  // CS = 1
  if(status_tr == SPI_OK) {
    // TODO
  }
}

// CS global
static const gpio_t cs = {GPIOB, 6};


int main(void) {
  SystemClock_Config();
  systick_config_ms(100);

  // init pins
  static const gpio_t sck = {GPIOA, 5};
  static const gpio_t miso = {GPIOA, 6};
  static const gpio_t mosi = {GPIOA, 7};
  
  // AF5 , PULL UP, OPEN DRAIN, SPEED HIGH
  gpio_init(&sck, GPIO_MODE_AF_t, GPIO_PULL_UP_t, 
    GPIO_OTYPE_PP_t, GPIO_SPEED_VERY_HIGH_t);
  gpio_init(&miso, GPIO_MODE_AF_t, GPIO_PULL_UP_t, 
    GPIO_OTYPE_PP_t, GPIO_SPEED_VERY_HIGH_t);
  gpio_init(&mosi, GPIO_MODE_AF_t, GPIO_PULL_UP_t, 
    GPIO_OTYPE_PP_t, GPIO_SPEED_VERY_HIGH_t);
  gpio_init(&cs, GPIO_MODE_OUTPUT_t, GPIO_PULL_UP_t, 
    GPIO_OTYPE_PP_t, GPIO_SPEED_VERY_HIGH_t);

  gpio_set_alternate_function(&sck, 5);
  gpio_set_alternate_function(&miso, 5);
  gpio_set_alternate_function(&mosi, 5);

  // spi interface
  static spi_t spi1 = {.instance = SPI1};

  spi_init(&spi1);

  // for test use SD card
  // Power On Sequence: CS = 1, send 10 times 0xFF. Expected response: R1_IDLE_STATE (0x01)
  
  uint8_t sd_seq[10] = {0xFF};
  spi_transaction_t power_on = {
    .tx_buff = sd_seq,
    .tx_len = sizeof(sd_seq),
    .rx_buff = NULL,
    .rx_len = 0,
    .callback = sd_callback
  };

  // set CS = 1
  cs.port->BSRR = CS_HIGH;
  // send x10 0xFF
  spi_execute_transaction(&spi1, &power_on);
  // get pesponse
  while(spi1.state != SPI_READY);


  while(1) {
    __NOP();
  }
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE3);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = 8;
  RCC_OscInitStruct.PLL.PLLN = 100;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 2;
  RCC_OscInitStruct.PLL.PLLR = 2;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_3) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
