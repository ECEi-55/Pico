#include "motor.h"
#include "pico/stdlib.h"
#include <hardware/pwm.h>

#define MAX_WRAP 255

void init_motor(motor_t *motor, uint pwmPin, uint fwdPin, uint revPin){
    // Configure the pico pins
    gpio_init(pwmPin);
    gpio_set_dir(pwmPin, GPIO_OUT);
    gpio_set_function(pwmPin, GPIO_FUNC_PWM);
    // Get PWM specifics
    uint slice = pwm_gpio_to_slice_num(pwmPin);
    uint channel = pwm_gpio_to_channel(pwmPin);
    // Enable the PWM pin
    pwm_set_enabled(slice, true);
    // Set phase correction false to count then reset to 0
    pwm_set_phase_correct(slice, false);
    // Set clock divider to get low operating point
    // 125MHz / (250 + 0/16) = 500kHz
    pwm_set_clkdiv_int_frac(slice, 250, 0);
    // Set count wraparound to further lower operating point
    // Actual frequency is fc/(MAX_WRAP+1) (~2kHz with MAX_WRAP = 255)
    pwm_set_wrap(slice, MAX_WRAP);
 
    gpio_init(fwdPin);
    gpio_set_dir(fwdPin, GPIO_OUT);
    gpio_init(revPin);
    gpio_set_dir(revPin, GPIO_OUT);

    motor->_pwmPin = pwmPin;
    motor->_fwdPin = fwdPin;
    motor->_revPin = revPin;

    // Set motor to stopped
    set_motor(motor, 0);
}

void set_motor(motor_t *motor, float output) {
    gpio_put(motor->_fwdPin, output > 0);
    gpio_put(motor->_revPin, output < 0);

    // For PWM output take abs, cap at max, then 
    output = fabs(output);
    output = output > MAX_WRAP ? MAX_WRAP : output;
    pwm_set_gpio_level(motor->_pwmPin, output * MAX_WRAP);
}