
#include "main.h"
#include "cmsis_gcc.h"
#include "gpio.h"
#include "stm32f446xx.h"
#include "stm32f4xx_hal_gpio.h"
#include "systick.h"
#include "timer.h"
#include <string.h>
#include <usart.h>
#include <stdint.h>
#include <i2c.h>
#include <stdio.h>

void SystemClock_Config(void);

bool ping_oled(void) {
    I2C1->CR1 |= I2C_CR1_START; // generate START
    // wait flag SB in SR1
    uint32_t timeout = 10000;
    while (!(I2C1->SR1 & I2C_SR1_SB)) if (--timeout == 0) return false;
    // send OLED adress
    I2C1->DR = 0x78;
    timeout = 10000;
    // wait flag ADDR in SR1
    while(!(I2C1->SR1 & I2C_SR1_ADDR)) {
        if(I2C1->SR1 & I2C_SR1_AF) { // if get NACK i.e. OLED not responsed
            I2C1->CR1 |= I2C_CR1_STOP;
            return false;
        }
        if(--timeout == 0) return false;
    }
    // reset flag ADDR (stm32 specific)
    (void)I2C1->SR1;
    (void)I2C1->SR2;
    // generate STOP
    I2C1->CR1 |= I2C_CR1_STOP;
    return true; // display OK
}

int main(void) {
  SystemClock_Config();
  systick_config_ms(100);

  static const gpio_t usart2_pa2 = {GPIOA, 2};
  static const gpio_t usart2_pa3 = {GPIOA, 3};

  gpio_init(&usart2_pa2, GPIO_MODE_AF_t, GPIO_PULL_UP_t, 
    GPIO_OTYPE_PP_t, GPIO_SPEED_HIGH_t);
  gpio_init(&usart2_pa3, GPIO_MODE_AF_t, GPIO_PULL_UP_t, 
    GPIO_OTYPE_PP_t, GPIO_SPEED_HIGH_t);

  gpio_set_alternate_function(&usart2_pa2, 7);
  gpio_set_alternate_function(&usart2_pa3, 7);
  
  usart_t usart_1 = {
    .instance = USART2,
    .bus_freq = 25000000
  };

  usart_init(&usart_1, 115200);

  // Init I2C
  static const gpio_t i2c1_sda = {GPIOB, 9};
  static const gpio_t i2c1_scl = {GPIOB, 8};

  gpio_init(&i2c1_sda, GPIO_MODE_AF_t, GPIO_PULL_UP_t,
     GPIO_OTYPE_OD_t, GPIO_SPEED_HIGH_t);
  gpio_init(&i2c1_scl, GPIO_MODE_AF_t, GPIO_PULL_UP_t,
     GPIO_OTYPE_OD_t, GPIO_SPEED_HIGH_t);

  gpio_set_alternate_function(&i2c1_sda, 4);
  gpio_set_alternate_function(&i2c1_scl, 4);

  static i2c_t i2c1 = {
    .bus = I2C1
  };

  i2c_init(&i2c1, 25, I2C_MODE_STANDARD_100KHZ);

  static uint8_t tx_buff[128] = {0};
  static uint8_t rx_buff[128] = {0};

  i2c_transaction_t wrong_tr = {
    .addr = 0x7F,
    .tx_buff = tx_buff,
    .tx_len = 128,
    .rx_buff = rx_buff,
    .rx_len = 128,
    .repeated_start = false
  };

  uint8_t oled_cmd[] = {0x00, 0xAF};

  i2c_transaction_t oled_tr = {
      .addr = 0x3C,          // Чистый 7-битный адрес!
      .tx_buff = oled_cmd,
      .tx_len = 2,
      .rx_buff = NULL,
      .rx_len = 0,
      .repeated_start = false
  };

  //i2c_execute(&i2c1, &oled_tr);


  
  while(1) {
    ping_oled();
    delay_ms(500);
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
