/**
 * @file motor_driver.c
 * @brief Low-level motor driver for ESP32 + L298N
 *
 * Responsibilities:
 *  - Configure GPIO direction pins
 *  - Configure LEDC PWM
 *  - Apply direction and PWM safely
 *
 * This module does NOT:
 *  - Perform PID control
 *  - Read encoder values
 *  - Implement safety logic
 *  - Handle UART communication
 *
 * It is purely a hardware abstraction layer.
 */

#include "motor_driver.h"
#include "motor_config.h"

#include "driver/gpio.h"
#include "driver/ledc.h"
#include "esp_err.h"
#include <stdlib.h>   // for abs()


/* =========================================================
 * Static Driver State
 * ---------------------------------------------------------
 * These variables store the last applied values.
 * They are NOT control variables — only status tracking.
 * ========================================================= */

static uint32_t left_duty = 0;
static uint32_t right_duty = 0;

static motor_direction_t left_direction = MOTOR_STOP;
static motor_direction_t right_direction = MOTOR_STOP;


/* =========================================================
 * INTERNAL INITIALIZATION FUNCTIONS
 * ========================================================= */

/**
 * @brief Configure GPIO pins for motor direction control.
 *
 * Sets IN1/IN2 pins as output.
 * Default state is LOW (motor stopped).
 */
static void motor_gpio_init(void)
{
    gpio_config_t io_conf = {
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };

    // Configure Motor 1 direction pins
    io_conf.pin_bit_mask = (1ULL << M1_IN1_GPIO) |
                           (1ULL << M1_IN2_GPIO);
    gpio_config(&io_conf);

    // Configure Motor 2 direction pins
    io_conf.pin_bit_mask = (1ULL << M2_IN1_GPIO) |
                           (1ULL << M2_IN2_GPIO);
    gpio_config(&io_conf);

    // Ensure both motors are stopped initially
    gpio_set_level(M1_IN1_GPIO, 0);
    gpio_set_level(M1_IN2_GPIO, 0);
    gpio_set_level(M2_IN1_GPIO, 0);
    gpio_set_level(M2_IN2_GPIO, 0);
}


/**
 * @brief Configure LEDC PWM timer and channels.
 *
 * One timer is shared by both motors.
 * Each motor gets its own channel.
 */
static void motor_pwm_init(void)
{
    // Configure PWM timer
    ledc_timer_config_t timer_conf = {
        .speed_mode = LEDC_HIGH_SPEED_MODE,
        .timer_num = LEDC_TIMER_0,
        .duty_resolution = MOTOR_PWM_RESOLUTION_BITS,
        .freq_hz = MOTOR_PWM_FREQUENCY_HZ,
        .clk_cfg = LEDC_AUTO_CLK
    };

    ledc_timer_config(&timer_conf);

    // Configure LEFT motor PWM channel
    ledc_channel_config_t left_channel = {
        .gpio_num = M1_PWM_GPIO,
        .speed_mode = LEDC_HIGH_SPEED_MODE,
        .channel = LEDC_CHANNEL_0,
        .timer_sel = LEDC_TIMER_0,
        .duty = 0,
        .hpoint = 0
    };

    ledc_channel_config(&left_channel);

    // Configure RIGHT motor PWM channel
    ledc_channel_config_t right_channel = {
        .gpio_num = M2_PWM_GPIO,
        .speed_mode = LEDC_HIGH_SPEED_MODE,
        .channel = LEDC_CHANNEL_1,
        .timer_sel = LEDC_TIMER_0,
        .duty = 0,
        .hpoint = 0
    };

    ledc_channel_config(&right_channel);
}


/* =========================================================
 * INTERNAL APPLY FUNCTIONS
 * ========================================================= */

/**
 * @brief Apply motor direction via IN1/IN2 pins.
 *
 * Forward:
 *    IN1 = 1
 *    IN2 = 0
 *
 * Reverse:
 *    IN1 = 0
 *    IN2 = 1
 *
 * Stop:
 *    IN1 = 0
 *    IN2 = 0
 */
static void apply_motor_direction(motor_id_t motor,
                                  motor_direction_t dir)
{
    if (motor == MOTOR_LEFT)
    {
        if (dir == MOTOR_FORWARD)
        {
            gpio_set_level(M1_IN1_GPIO, 1);
            gpio_set_level(M1_IN2_GPIO, 0);
        }
        else if (dir == MOTOR_REVERSE)
        {
            gpio_set_level(M1_IN1_GPIO, 0);
            gpio_set_level(M1_IN2_GPIO, 1);
        }
        else
        {
            gpio_set_level(M1_IN1_GPIO, 0);
            gpio_set_level(M1_IN2_GPIO, 0);
        }

        left_direction = dir;
    }
    else
    {
        if (dir == MOTOR_FORWARD)
        {
            gpio_set_level(M2_IN1_GPIO, 1);
            gpio_set_level(M2_IN2_GPIO, 0);
        }
        else if (dir == MOTOR_REVERSE)
        {
            gpio_set_level(M2_IN1_GPIO, 0);
            gpio_set_level(M2_IN2_GPIO, 1);
        }
        else
        {
            gpio_set_level(M2_IN1_GPIO, 0);
            gpio_set_level(M2_IN2_GPIO, 0);
        }

        right_direction = dir;
    }
}


/**
 * @brief Apply PWM duty to selected motor.
 *
 * Duty is automatically clamped to MOTOR_PWM_MAX_DUTY.
 */
static void apply_motor_pwm(motor_id_t motor,
                            uint32_t duty)
{
    // Clamp duty to safe limits
    if (duty > MOTOR_PWM_MAX_DUTY)
        duty = MOTOR_PWM_MAX_DUTY;

    if (motor == MOTOR_LEFT)
    {
        ledc_set_duty(LEDC_HIGH_SPEED_MODE,
                      LEDC_CHANNEL_0,
                      duty);

        ledc_update_duty(LEDC_HIGH_SPEED_MODE,
                         LEDC_CHANNEL_0);

        left_duty = duty;
    }
    else
    {
        ledc_set_duty(LEDC_HIGH_SPEED_MODE,
                      LEDC_CHANNEL_1,
                      duty);

        ledc_update_duty(LEDC_HIGH_SPEED_MODE,
                         LEDC_CHANNEL_1);

        right_duty = duty;
    }
}


/* =========================================================
 * PUBLIC API IMPLEMENTATION
 * ========================================================= */

/**
 * @brief Initialize motor driver hardware.
 *
 * Must be called once during system startup.
 */
void motor_driver_init(void)
{
    motor_gpio_init();
    motor_pwm_init();

    // Ensure motors are off after initialization
    motor_disable_all();
}


/**
 * @brief Set motor direction only.
 */
void motor_set_direction(motor_id_t motor,
                         motor_direction_t direction)
{
    apply_motor_direction(motor, direction);
}


/**
 * @brief Set motor PWM duty only.
 */
void motor_set_duty(motor_id_t motor,
                    uint32_t duty)
{
    apply_motor_pwm(motor, duty);
}


/**
 * @brief Main control API.
 *
 * Accepts signed control value:
 *   > 0 → Forward
 *   < 0 → Reverse
 *   = 0 → Stop
 *
 * Direction is applied BEFORE PWM to prevent
 * shoot-through current spikes.
 */
void motor_set_output(motor_id_t motor,
                      int32_t signed_output)
{
    if (signed_output == 0)
    {
        apply_motor_direction(motor, MOTOR_STOP);
        apply_motor_pwm(motor, 0);
        return;
    }

    motor_direction_t direction =
        (signed_output > 0) ? MOTOR_FORWARD : MOTOR_REVERSE;

    uint32_t duty = (uint32_t) abs(signed_output);

    if (duty > MOTOR_PWM_MAX_DUTY)
        duty = MOTOR_PWM_MAX_DUTY;

    // Apply direction first
    apply_motor_direction(motor, direction);

    // Then apply PWM
    apply_motor_pwm(motor, duty);
}


/**
 * @brief Stop a single motor.
 */
void motor_stop(motor_id_t motor)
{
    apply_motor_pwm(motor, 0);
    apply_motor_direction(motor, MOTOR_STOP);
}


/**
 * @brief Emergency stop for all motors.
 */
void motor_disable_all(void)
{
    motor_stop(MOTOR_LEFT);
    motor_stop(MOTOR_RIGHT);
}


/**
 * @brief Get last applied PWM duty.
 */
uint32_t motor_get_duty(motor_id_t motor)
{
    return (motor == MOTOR_LEFT) ? left_duty : right_duty;
}


/**
 * @brief Get last applied direction.
 */
motor_direction_t motor_get_direction(motor_id_t motor)
{
    return (motor == MOTOR_LEFT) ? left_direction : right_direction;
}
