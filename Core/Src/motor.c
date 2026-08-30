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

extern ADC_HandleTypeDef hadc2;
extern TIM_HandleTypeDef htim1,htim2;

static InputFilter_t STOPFilter;
static InputFilter_t OTVFilter;
static InputFilter_t INKREMFilter;

static InputFilter_t KON_ZFilter;
static InputFilter_t KON_OFilter;
static InputFilter_t ZAVFilter;



static void InputFilter_Update(InputFilter_t *filter, uint8_t sample);
void Motor_Control_1ms(void);

static uint16_t Motor_ADC2_Read(void)
{
    uint16_t value = 0;

    if (HAL_ADC_Start(&hadc2) == HAL_OK)
    {
        if (HAL_ADC_PollForConversion(&hadc2, 1) == HAL_OK)
        {
            value = (uint16_t)HAL_ADC_GetValue(&hadc2);
        }

        HAL_ADC_Stop(&hadc2);
    }

    return value;
}

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
 {
	/* -------------------------------------------------
	       TIM2 - testovací výstup 0,5 s
	       ------------------------------------------------- */
      if (htim->Instance == TIM2)
      {
          HAL_GPIO_TogglePin(OUTEST_GPIO_Port, OUTEST_Pin);
          uint16_t adc2 = Motor_ADC2_Read();

             // zde s adc2 můžete dále pracovat
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

         //     HAL_GPIO_WritePin(OUTEST_GPIO_Port, OUTEST_Pin,
         //                       GetSTOP() || GetZAV() || GetOTV());	//t
              Motor_Control_1ms();//t
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
   // return (( ~STOPFilter.state) & 0x1);
    return (STOPFilter.state);
}

uint8_t GetOTV(void)
{
   // return (( ~OTVFilter.state) & 0x1);
    return (OTVFilter.state);
}

uint8_t GetINKREM(void)
{
    return INKREMFilter.state;
}

uint8_t GetZAV(void)
{
    //return (( ~ZAVFilter.state) & 0x1);
    return (ZAVFilter.state);
}

uint8_t GetKON_Z(void)
{
    return KON_ZFilter.state;
}

uint8_t GetKON_O(void)
{
    return KON_OFilter.state;
}

/* -------------------------------------------------------------------------
 * Motor ramp control
 *
 * Motor_Control_1ms() is called every 1 ms from TIM3.
 *
 * Speed range:
 *   0       = motor stopped
 *   1000    = maximum PWM
 *
 * STARTSPEED = speed at the beginning of an acceleration ramp
 * STOPSPEED  = final low speed before switching the PWM off
 *
 * STARTRAMPA and STOPRAMPA are in seconds and may be 1 ... 10 s.
 * ------------------------------------------------------------------------- */

#define STARTSPEED       100U
#define STOPSPEED        100U

#define STARTRAMPA       3U       /* acceleration time [s] */
#define STOPRAMPA        2U       /* deceleration time [s] */

#define MOTOR_MAX_SPEED 1000U
#define MOTOR_SUPER_SPEED 1024U


typedef enum
{
    MOTOR_STATE_STOPPED = 0,

    MOTOR_STATE_ACCEL_FORWARD,
    MOTOR_STATE_RUN_FORWARD,
    MOTOR_STATE_DECEL_FORWARD,

    MOTOR_STATE_ACCEL_REVERSE,
    MOTOR_STATE_RUN_REVERSE,
    MOTOR_STATE_DECEL_REVERSE

} MotorState_t;


static MotorState_t motorState = MOTOR_STATE_STOPPED;

static uint16_t motorSpeed = 0;
static uint16_t rampStartSpeed = 0;
static uint32_t rampCounter = 0;


/* Linear ramp from start to target.
 * elapsed_ms must be in the range 0 ... duration_ms.
 */
static uint16_t Motor_Ramp(uint16_t start,
                           uint16_t target,
                           uint32_t duration_ms,
                           uint32_t elapsed_ms)
{
    if (duration_ms == 0U)
        return target;

    if (elapsed_ms >= duration_ms)
        return target;

    if (target > start)
    {
        return (uint16_t)(
            start +
            ((uint32_t)(target - start) * elapsed_ms) / duration_ms
        );
    }

    return (uint16_t)(
        start -
        ((uint32_t)(start - target) * elapsed_ms) / duration_ms
    );
}


/* Low-level PWM control. */
void Motor_SetSpeed(MotorDirection dir, uint16_t speed)
{
    if (speed > MOTOR_MAX_SPEED)
        speed = MOTOR_MAX_SPEED;

    uint16_t pwm =
        (uint16_t)((uint32_t)speed *
                   (__HAL_TIM_GET_AUTORELOAD(&htim1) + 1U) /
                   MOTOR_SUPER_SPEED);

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


        default:

            __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_2, 0);
            __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_3, 0);
            break;
    }
}


/* -------------------------------------------------------------------------
 * Motor control, called every 1 ms.
 * ------------------------------------------------------------------------- */
void Motor_Control_1ms(void)
{
    uint32_t rampTime_ms;


    switch (motorState)
    {
        /* -------------------------------------------------------------
         * MOTOR STOPPED
         * ------------------------------------------------------------- */
        case MOTOR_STATE_STOPPED:

            motorSpeed = 0;
            rampCounter = 0;

            Motor_SetSpeed(MOTOR_STOP, 0);

            /*
             * STOP has the highest priority.
             */
            if (GetSTOP())
                break;

            /*
             * Start forward.
             */
            if (GetOTV() && !GetKON_O())
            {
                motorSpeed = STARTSPEED;
                rampStartSpeed = STARTSPEED;
                rampCounter = 0;

                Motor_SetSpeed(MOTOR_FORWARD, motorSpeed);

                motorState = MOTOR_STATE_ACCEL_FORWARD;
            }

            /*
             * Start reverse.
             */
            else if (GetZAV() && !GetKON_Z())
            {
                motorSpeed = STARTSPEED;
                rampStartSpeed = STARTSPEED;
                rampCounter = 0;

                Motor_SetSpeed(MOTOR_REVERSE, motorSpeed);

                motorState = MOTOR_STATE_ACCEL_REVERSE;
            }

            break;


        /* -------------------------------------------------------------
         * ACCELERATION FORWARD
         * ------------------------------------------------------------- */
        case MOTOR_STATE_ACCEL_FORWARD:

            /*
             * Loss of OTV, STOP or forward limit switch:
             * immediately change to controlled deceleration.
             */
            if (GetSTOP() || !GetOTV() || GetKON_O())
            {
                rampStartSpeed = motorSpeed;
                rampCounter = 0;

                motorState = MOTOR_STATE_DECEL_FORWARD;
                break;
            }

            rampTime_ms = (uint32_t)STARTRAMPA * 1000U;

            motorSpeed = Motor_Ramp(STARTSPEED,
                                    MOTOR_MAX_SPEED,
                                    rampTime_ms,
                                    rampCounter);

            Motor_SetSpeed(MOTOR_FORWARD, motorSpeed);

            if (rampCounter < rampTime_ms)
                rampCounter++;
            else
                motorState = MOTOR_STATE_RUN_FORWARD;

            break;


        /* -------------------------------------------------------------
         * FULL SPEED FORWARD
         * ------------------------------------------------------------- */
        case MOTOR_STATE_RUN_FORWARD:

            if (GetSTOP() || !GetOTV() || GetKON_O())
            {
                /*
                 * Save the actual speed at the instant deceleration starts.
                 */
                rampStartSpeed = motorSpeed;
                rampCounter = 0;

                motorState = MOTOR_STATE_DECEL_FORWARD;
                break;
            }

            motorSpeed = MOTOR_MAX_SPEED;
            Motor_SetSpeed(MOTOR_FORWARD, motorSpeed);

            break;


        /* -------------------------------------------------------------
         * DECELERATION FORWARD
         * ------------------------------------------------------------- */
        case MOTOR_STATE_DECEL_FORWARD:

            rampTime_ms = (uint32_t)STOPRAMPA * 1000U;

            motorSpeed = Motor_Ramp(rampStartSpeed,
                                    STOPSPEED,
                                    rampTime_ms,
                                    rampCounter);

            Motor_SetSpeed(MOTOR_FORWARD, motorSpeed);

            if (rampCounter < rampTime_ms)
            {
                rampCounter++;
            }
            else
            {
                motorSpeed = 0;

                Motor_SetSpeed(MOTOR_STOP, 0);

                rampCounter = 0;
                motorState = MOTOR_STATE_STOPPED;
            }

            break;


        /* -------------------------------------------------------------
         * ACCELERATION REVERSE
         * ------------------------------------------------------------- */
        case MOTOR_STATE_ACCEL_REVERSE:

            if (GetSTOP() || !GetZAV() || GetKON_Z())
            {
                rampStartSpeed = motorSpeed;
                rampCounter = 0;

                motorState = MOTOR_STATE_DECEL_REVERSE;
                break;
            }

            rampTime_ms = (uint32_t)STARTRAMPA * 1000U;

            motorSpeed = Motor_Ramp(STARTSPEED,
                                    MOTOR_MAX_SPEED,
                                    rampTime_ms,
                                    rampCounter);

            Motor_SetSpeed(MOTOR_REVERSE, motorSpeed);

            if (rampCounter < rampTime_ms)
                rampCounter++;
            else
                motorState = MOTOR_STATE_RUN_REVERSE;

            break;


        /* -------------------------------------------------------------
         * FULL SPEED REVERSE
         * ------------------------------------------------------------- */
        case MOTOR_STATE_RUN_REVERSE:

            if (GetSTOP() || !GetZAV() || GetKON_Z())
            {
                /*
                 * Save the actual speed at the instant deceleration starts.
                 */
                rampStartSpeed = motorSpeed;
                rampCounter = 0;

                motorState = MOTOR_STATE_DECEL_REVERSE;
                break;
            }

            motorSpeed = MOTOR_MAX_SPEED;
            Motor_SetSpeed(MOTOR_REVERSE, motorSpeed);

            break;


        /* -------------------------------------------------------------
         * DECELERATION REVERSE
         * ------------------------------------------------------------- */
        case MOTOR_STATE_DECEL_REVERSE:

            rampTime_ms = (uint32_t)STOPRAMPA * 1000U;

            motorSpeed = Motor_Ramp(rampStartSpeed,
                                    STOPSPEED,
                                    rampTime_ms,
                                    rampCounter);

            Motor_SetSpeed(MOTOR_REVERSE, motorSpeed);

            if (rampCounter < rampTime_ms)
            {
                rampCounter++;
            }
            else
            {
                motorSpeed = 0;

                Motor_SetSpeed(MOTOR_STOP, 0);

                rampCounter = 0;
                motorState = MOTOR_STATE_STOPPED;
            }

            break;


        default:

            motorSpeed = 0;
            rampStartSpeed = 0;
            rampCounter = 0;

            motorState = MOTOR_STATE_STOPPED;

            Motor_SetSpeed(MOTOR_STOP, 0);

            break;
    }
}
