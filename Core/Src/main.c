
#include "main.h"
#include "gpio.h"
#include "stm32f446xx.h"
#include "systick.h"
#include "timer.h"
#include <stdint.h>

void SystemClock_Config(void);


int main(void) {
  SystemClock_Config();
  systick_config_ms(100);

  static const gpio_t b_led = {GPIOA, 0};
  static const gpio_t r_led = {GPIOA, 1};
  static const gpio_t w_led = {GPIOB, 10};
  static const gpio_t g_led = {GPIOB, 3};


  gpio_init(&b_led, GPIO_MODE_AF_t, GPIO_PULL_NONE_t, 
    GPIO_OTYPE_PP_t, GPIO_SPEED_HIGH_t);
  gpio_init(&r_led, GPIO_MODE_AF_t, GPIO_PULL_NONE_t, 
    GPIO_OTYPE_PP_t, GPIO_SPEED_HIGH_t);
  gpio_init(&w_led, GPIO_MODE_AF_t, GPIO_PULL_NONE_t, 
    GPIO_OTYPE_PP_t, GPIO_SPEED_HIGH_t);
  gpio_init(&g_led, GPIO_MODE_AF_t, GPIO_PULL_NONE_t, 
    GPIO_OTYPE_PP_t, GPIO_SPEED_HIGH_t);

  gpio_set_alternate_function(&b_led, 1);
  gpio_set_alternate_function(&r_led, 1);
  gpio_set_alternate_function(&w_led, 1);
  gpio_set_alternate_function(&g_led, 1);

  // setup timers
  timer_general_t tim2 = {TIM2, 50};
  // init
  timer_general_init_ms(&tim2, 150, 0);
  timer_general_pwm_channel_config(&tim2, 1);
  timer_general_pwm_channel_config(&tim2, 2);
  timer_general_pwm_channel_config(&tim2, 3);
  timer_general_pwm_channel_config(&tim2, 4);
  // dc
  timer_general_set_duty(&tim2, 1, 10);
  timer_general_set_duty(&tim2, 2, 20);
  timer_general_set_duty(&tim2, 3, 30);
  timer_general_set_duty(&tim2, 4, 40);

  timer_general_start(&tim2);

  int8_t step = 1;     // Шаг изменения (1%)
  uint8_t duty = 0;    // Текущая скважность

  while (1) {
      // 1. Сначала обновляем железо
      timer_general_set_duty(&tim2, 1, duty);
      timer_general_set_duty(&tim2, 2, duty);
      timer_general_set_duty(&tim2, 3, duty);
      timer_general_set_duty(&tim2, 4, duty);
      
      duty += step;


      if (duty >= 100) {
          duty = 100;
          step = -1;
      } else if (duty <= 0) {
          duty = 0;
          step = 1;
      }

      delay_ms(20); 
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
