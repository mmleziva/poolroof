/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32g4xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

void HAL_TIM_MspPostInit(TIM_HandleTypeDef *htim);

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define OUTEST_Pin GPIO_PIN_4
#define OUTEST_GPIO_Port GPIOA
#define STOP_Pin GPIO_PIN_8
#define STOP_GPIO_Port GPIOA
#define OTV_Pin GPIO_PIN_11
#define OTV_GPIO_Port GPIOA
#define INKREM_Pin GPIO_PIN_15
#define INKREM_GPIO_Port GPIOA
#define KON_Z_Pin GPIO_PIN_3
#define KON_Z_GPIO_Port GPIOB
#define KON_O_Pin GPIO_PIN_4
#define KON_O_GPIO_Port GPIOB
#define ZAV_Pin GPIO_PIN_5
#define ZAV_GPIO_Port GPIOB

/* USER CODE BEGIN Private defines */
#define ADC1_DMA_BUFFER_LENGTH 100U
extern ADC_ChannelConfTypeDef newConfig;
extern volatile uint16_t ADC1_LastValue;
extern volatile uint16_t ADC1_DMA_Buffer[ADC1_DMA_BUFFER_LENGTH];
/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
