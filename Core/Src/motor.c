/*
 * motor.c
 *
 *  Created on: 10. 8. 2026
 *      Author: DELL
 */

#include "motor.h"

typedef struct
{
    uint8_t history;
    uint8_t state;
} InputFilter_t;

#define INPUT_FILTER_SAMPLES     8U
#define INPUT_FILTER_THRESHOLD   6U

extern TIM_HandleTypeDef htim1,htim2;

static InputFilter_t STOPFilter;
static InputFilter_t OTVFilter;
static InputFilter_t INKREMFilter;

static InputFilter_t KON_ZFilter;
static InputFilter_t KON_OFilter;
static InputFilter_t ZAVFilter;
static void InputFilter_Update(InputFilter_t *filter, uint8_t sample);

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
 {
	/* -------------------------------------------------
	       TIM2 - testovací výstup 0,5 s
	       ------------------------------------------------- */
      if (htim->Instance == TIM2)
      {
          HAL_GPIO_TogglePin(OUTEST_GPIO_Port, OUTEST_Pin);
      }
      /* -------------------------------------------------
             TIM3 - vzorkování vstupů každou 1 ms
             ------------------------------------------------- */

      if (htim->Instance == TIM3)
      {
              InputFilter_Update(&STOPFilter,
                                 !HAL_GPIO_ReadPin(STOP_GPIO_Port, STOP_Pin));

              InputFilter_Update(&OTVFilter,
                                 !HAL_GPIO_ReadPin(OTV_GPIO_Port, OTV_Pin));

              InputFilter_Update(&INKREMFilter,
                                 !HAL_GPIO_ReadPin(INKREM_GPIO_Port, INKREM_Pin));


              InputFilter_Update(&KON_ZFilter,
                                 !HAL_GPIO_ReadPin(KON_Z_GPIO_Port, KON_Z_Pin));

              InputFilter_Update(&KON_OFilter,
                                 !HAL_GPIO_ReadPin(KON_O_GPIO_Port, KON_O_Pin));

              InputFilter_Update(&ZAVFilter,
                                 !HAL_GPIO_ReadPin(ZAV_GPIO_Port, ZAV_Pin));
      }
 }

static void InputFilter_Update(InputFilter_t *filter, uint8_t sample)
{
    filter->history <<= 1;
    filter->history |= sample;

    uint8_t ones = __builtin_popcount(filter->history);

    if (filter->state == 0)
    {
        if (ones >= 6)
            filter->state = 1;
    }
    else
    {
        if (ones <= 2)
            filter->state = 0;
    }
}

uint8_t GetSTOP(void)
{
    return STOPFilter.state;
}

uint8_t GetOTV(void)
{
    return OTVFilter.state;
}

uint8_t GetINKREM(void)
{
    return INKREMFilter.state;
}

uint8_t GetZAV(void)
{
    return ZAVFilter.state;
}

uint8_t GetKON_Z(void)
{
    return KON_ZFilter.state;
}

uint8_t GetKON_O(void)
{
    return KON_OFilter.state;
}

void Motor_SetSpeed(MotorDirection dir, uint16_t speed)
{
    if(speed > 1000)
        speed = 1000;

    uint16_t pwm = speed * (__HAL_TIM_GET_AUTORELOAD(&htim1) + 1) / 1000;

    switch(dir)
    {
        case MOTOR_STOP:

            __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_2, 0);
            __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_3, 0);
            break;

        case MOTOR_FORWARD:

            __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_3, 0);
            __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_2, pwm);
            break;

        case MOTOR_REVERSE:

            __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_2, 0);
            __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_3, pwm);
            break;
    }
}
