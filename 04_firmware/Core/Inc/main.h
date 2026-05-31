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
#include "stm32h7xx_hal.h"

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

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define imu_int1_Pin GPIO_PIN_4
#define imu_int1_GPIO_Port GPIOF
#define imu_int1_EXTI_IRQn EXTI4_IRQn
#define imu_int2_Pin GPIO_PIN_5
#define imu_int2_GPIO_Port GPIOF
#define imu_int2_EXTI_IRQn EXTI9_5_IRQn
#define ibus_sense_Pin GPIO_PIN_0
#define ibus_sense_GPIO_Port GPIOC
#define vbat_sense_Pin GPIO_PIN_3
#define vbat_sense_GPIO_Port GPIOA
#define imu_spi_sck_Pin GPIO_PIN_5
#define imu_spi_sck_GPIO_Port GPIOA
#define imu_spi_miso_Pin GPIO_PIN_6
#define imu_spi_miso_GPIO_Port GPIOA
#define debug_tx_Pin GPIO_PIN_8
#define debug_tx_GPIO_Port GPIOD
#define debug_rx_Pin GPIO_PIN_9
#define debug_rx_GPIO_Port GPIOD
#define imu_cs_n_Pin GPIO_PIN_14
#define imu_cs_n_GPIO_Port GPIOD
#define fdcan1_rx_Pin GPIO_PIN_0
#define fdcan1_rx_GPIO_Port GPIOD
#define fdcan1_tx_Pin GPIO_PIN_1
#define fdcan1_tx_GPIO_Port GPIOD
#define crsf_tx_Pin GPIO_PIN_5
#define crsf_tx_GPIO_Port GPIOD
#define crsf_rx_Pin GPIO_PIN_6
#define crsf_rx_GPIO_Port GPIOD
#define imu_spi_mosi_Pin GPIO_PIN_5
#define imu_spi_mosi_GPIO_Port GPIOB
#define oled_i2c_scl_Pin GPIO_PIN_8
#define oled_i2c_scl_GPIO_Port GPIOB
#define oled_i2c_sda_Pin GPIO_PIN_9
#define oled_i2c_sda_GPIO_Port GPIOB

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
