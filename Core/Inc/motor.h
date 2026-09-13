/*
 * motor.h
 *
 *  Created on: 10. 8. 2026
 *      Author: DELL
 */

#ifndef INC_MOTOR_H_
#define INC_MOTOR_H_

#include "stm32g4xx_hal.h"
#include "main.h"

uint8_t GetSTOP(void);
uint8_t GetOTV(void);
uint8_t GetINKREM(void);

uint8_t GetKON_Z(void);
uint8_t GetKON_O(void);
uint8_t GetZAV(void);

extern volatile uint16_t CTPULS;

typedef enum
{
    MOTOR_STOP = 0,
    MOTOR_FORWARD,
    MOTOR_REVERSE
} MotorDirection;

void Motor_SetSpeed(MotorDirection dir, uint16_t speed);

#endif /* INC_MOTOR_H_ */
