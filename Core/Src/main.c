
#include "main.h"
#include "gpio.h"
#include "stm32f446xx.h"
#include "systick.h"
#include "timer.h"
#include <stdint.h>

void SystemClock_Config(void);

// timer interrupt
void led_toggle_handler(void *context) {
  gpio_t *led = (gpio_t *)context;
  gpio_toggle(led);
}

int main(void) {
  SystemClock_Config();
  systick_config_ms(100);

  static const gpio_t b_led = {GPIOC, 0};
  static const gpio_t r_led = {GPIOC, 1};

  gpio_init_output(&b_led, true);
  gpio_init_output(&r_led, true);

  // setup timers
  timer_basic_t tim6 = {TIM6, 50};
  timer_basic_t tim7 = {TIM7, 50};
  // init
  timer_basic_init_ms(&tim6, 500);
  timer_basic_init_ms(&tim7, 1000);
  // add callbacks
  timer_basic_set_callback(TIM6, led_toggle_handler, &b_led);
  timer_basic_set_callback(TIM7, led_toggle_handler, &r_led);
  // start
  timer_basic_start(&tim6);
  timer_basic_start(&tim7);

  bool fast_mode = true;

while (1) {
    delay_ms(5000);
    
    if (fast_mode) {
        timer_basic_set_period_ms(&tim6, 20);
        timer_basic_set_period_ms(&tim7, 50);
    } else {
        timer_basic_set_period_ms(&tim6, 100);
        timer_basic_set_period_ms(&tim7, 200);
    }
    
    fast_mode = !fast_mode; // Меняем режим для следующей итерации
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
