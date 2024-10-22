/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2024 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "adc.h"
#include "gpdma.h"
#include "icache.h"
#include "memorymap.h"
#include "rtc.h"
#include "app_usbx_device.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "board.h"
#include "usb_otg.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
uint32_t adc_inp;
/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_GPDMA1_Init();
  MX_RTC_Init();
  MX_ICACHE_Init();
  MX_USBX_Device_Init();
  MX_ADC1_Init();
  /* USER CODE BEGIN 2 */
	MX_USB_OTG_FS_PCD_Init();
	
	HAL_PCDEx_SetRxFiFo(&hpcd_USB_OTG_FS, 0x80);
  HAL_PCDEx_SetTxFiFo(&hpcd_USB_OTG_FS, 0, USBD_MAX_EP0_SIZE / 4);
  HAL_PCDEx_SetTxFiFo(&hpcd_USB_OTG_FS, 1, USBD_CDCACM_EPIN_FS_MPS / 4);
  HAL_PCDEx_SetTxFiFo(&hpcd_USB_OTG_FS, 2, USBD_CDCACM_EPINCMD_FS_MPS / 4);

  _ux_dcd_stm32_initialize((ULONG)USB_OTG_FS, (ULONG)&hpcd_USB_OTG_FS);

	/* Start the USB device */
	HAL_PCD_Start(&hpcd_USB_OTG_FS);
	
	board_led_init();
	board_button_init();
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
	HAL_PWREx_EnableVddA();
	HAL_ADCEx_Calibration_Start(&hadc1,ADC_CALIB_OFFSET,ADC_SINGLE_ENDED);
	HAL_ADC_Start_DMA(&hadc1,(uint32_t *)&adc_inp,1);
	
	uint32_t tick = HAL_GetTick();
	uint32_t tick_usb = tick;
	uint32_t tick_usb_tx = tick;
	uint32_t tick_button = tick;
	
	uint8_t txbuf[50]; 
	ULONG length;
	ULONG actual_length;
	length = sprintf(( char *)txbuf,"Hello! WeAct Studio\r\n");
	UINT write_state = UX_STATE_RESET;
	UINT ux_status = UX_STATE_RESET;
  while (1)
  {
		tick = HAL_GetTick();
		if(tick >= tick_usb)
		{
			tick_usb = tick + 1;
			_ux_system_tasks_run();
		}
		
		if(tick >= tick_button)
		{
			if(board_button_getstate())
			{
				tick_button = tick + 100;
				board_led_toggle();
				length = sprintf(( char *)txbuf,"Key Pressed\r\n");
			}
			else
			{
				tick_button = tick + 500;
				RTC_DateTypeDef sdatestructureget;
        RTC_TimeTypeDef stimestructureget;
        static uint8_t Seconds_o;
        int text_lenth;
        
        /* Get the RTC current Time */
        HAL_RTC_GetTime(&hrtc, &stimestructureget, RTC_FORMAT_BIN);
        /* Get the RTC current Date */
        HAL_RTC_GetDate(&hrtc, &sdatestructureget, RTC_FORMAT_BIN);
        
        if(Seconds_o != stimestructureget.Seconds)
        {
          Seconds_o = stimestructureget.Seconds;
          
          board_led_set(1);
          
          length = sprintf((char *) &txbuf,"20%02d.%02d.%02d %02d:%02d %02d ,%dmV\r\n",sdatestructureget.Year,sdatestructureget.Month,sdatestructureget.Date, \
                                            stimestructureget.Hours,stimestructureget.Minutes,stimestructureget.Seconds,(((uint32_t)adc_inp)*3300)>>12);
        }
        else
        {
          board_led_set(0);
        }
			}
		}
		
		if(tick >= tick_usb_tx)
		{
			tick_usb_tx = tick + 1;
			
			extern UX_SLAVE_CLASS_CDC_ACM  *cdc_acm;
			if(cdc_acm != UX_NULL)
			{
				switch(write_state)
				{
				case UX_STATE_RESET:

					ux_status = ux_device_class_cdc_acm_write_run(cdc_acm, txbuf,length, &actual_length);
						
					if (ux_status != UX_STATE_WAIT)
					{
						/* Reset state.  */
						write_state = UX_STATE_RESET;
						break;
					}
					write_state = UX_STATE_WAIT;
					break;
						
				case UX_STATE_WAIT:    	
					/* Continue to run state machine.  */
					ux_status = ux_device_class_cdc_acm_write_run(cdc_acm, UX_NULL, 0, &actual_length);
					/* Check if there is  fatal error.  */
					if (ux_status < UX_STATE_IDLE)
					{
						/* Reset state.  */
						write_state = UX_STATE_RESET;
						break;
					}
					/* Check if dataset is transmitted */
					if (ux_status <= UX_STATE_NEXT)
					{
						write_state = UX_STATE_RESET;
						tick_usb_tx = tick + 1000;
					}
					/* Keep waiting.  */
					break;
				default:
					break;
				}
			}
		}
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
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
  if (HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure LSE Drive Capability
  */
  HAL_PWR_EnableBkUpAccess();
  __HAL_RCC_LSEDRIVE_CONFIG(RCC_LSEDRIVE_MEDIUMHIGH);

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI48|RCC_OSCILLATORTYPE_HSE
                              |RCC_OSCILLATORTYPE_LSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.LSEState = RCC_LSE_ON;
  RCC_OscInitStruct.HSI48State = RCC_HSI48_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMBOOST = RCC_PLLMBOOST_DIV2;
  RCC_OscInitStruct.PLL.PLLM = 5;
  RCC_OscInitStruct.PLL.PLLN = 64;
  RCC_OscInitStruct.PLL.PLLP = 2;
  RCC_OscInitStruct.PLL.PLLQ = 2;
  RCC_OscInitStruct.PLL.PLLR = 2;
  RCC_OscInitStruct.PLL.PLLRGE = RCC_PLLVCIRANGE_0;
  RCC_OscInitStruct.PLL.PLLFRACN = 0;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2
                              |RCC_CLOCKTYPE_PCLK3;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB3CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_4) != HAL_OK)
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

#ifdef  USE_FULL_ASSERT
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
