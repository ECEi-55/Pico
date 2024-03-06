#include <stdlib.h>
#include <hardware/pwm.h>
#ifndef MOTOR_H
#define MOTOR_H

typedef struct {
    uint _pwmPin;
    uint _fwdPin;
    uint _revPin;
} motor_t;

/**
 * @brief Initialize a new motor connection
 * 
 * @param motor Struct to initialize
 * @param pwmPin PWM control pin, ENA or ENB on l298n
 * @param fwdPin Forward input put, IN1 or IN3 on l298n
 * @param revPin Reverse input pin, IN2 or IN4 on l298n
 */
void motor_init(motor_t *motor, uint pwmPin, uint fwdPin, uint revPin);

/**
 * @brief Set the motor output
 * 
 * @param motor Motor connection to set
 * @param output Output percent, in range -1 to 1
 */
void motor_set(motor_t *motor, float output);

#endif