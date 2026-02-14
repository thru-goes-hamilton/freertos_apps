#ifndef MOTOR_DRIVER_H
#define MOTOR_DRIVER_H

#include <stdint.h>
#include <stdbool.h>
#include "motor_config.h"

/* =========================================================
 * Motor Driver Layer
 * ---------------------------------------------------------
 * Hardware abstraction for L298N motor control using ESP32.
 *
 * Responsibilities:
 *  - GPIO direction control
 *  - PWM generation via LEDC
 *  - Motor enable / disable
 *
 * Does NOT handle:
 *  - PID control
 *  - RPM calculations
 *  - UART communication
 *  - Safety state logic
 * ========================================================= */


/* =========================================================
 * ENUMERATIONS
 * ========================================================= */

/**
 * @brief Motor identifier
 */
typedef enum
{
    MOTOR_LEFT = 0,
    MOTOR_RIGHT
} motor_id_t;


/**
 * @brief Motor direction abstraction
 */
typedef enum
{
    MOTOR_STOP = 0,
    MOTOR_FORWARD,
    MOTOR_REVERSE
} motor_direction_t;


/* =========================================================
 * PUBLIC API
 * ========================================================= */

/**
 * @brief Initialize motor driver hardware
 *
 * - Configures GPIO pins
 * - Configures LEDC PWM timers and channels
 *
 * Must be called once at system startup.
 */
void motor_driver_init(void);


/**
 * @brief Set motor direction
 *
 * @param motor     Motor ID (LEFT or RIGHT)
 * @param direction Desired direction
 *
 * Does NOT modify PWM duty.
 */
void motor_set_direction(motor_id_t motor,
                         motor_direction_t direction);


/**
 * @brief Set PWM duty cycle for a motor
 *
 * @param motor     Motor ID
 * @param duty      PWM duty (0 to MOTOR_PWM_MAX_DUTY)
 *
 * Does NOT change direction.
 * Value is automatically clamped to safe limits.
 */
void motor_set_duty(motor_id_t motor,
                    uint32_t duty);


/**
 * @brief Set motor output using signed value
 *
 * @param motor     Motor ID
 * @param signed_output  Signed value:
 *                       > 0 → Forward
 *                       < 0 → Reverse
 *                       = 0 → Stop
 *
 * Magnitude is converted to PWM duty internally.
 * This is the main API used by motor_controller.c
 */
void motor_set_output(motor_id_t motor,
                      int32_t signed_output);


/**
 * @brief Stop a single motor (coast)
 *
 * Sets PWM to 0 and direction to STOP.
 */
void motor_stop(motor_id_t motor);


/**
 * @brief Disable all motors immediately
 *
 * Used by safety layer during FAULT condition.
 */
void motor_disable_all(void);


/* =========================================================
 * OPTIONAL (DEBUG / STATUS)
 * ========================================================= */

/**
 * @brief Get last applied PWM duty
 */
uint32_t motor_get_duty(motor_id_t motor);


/**
 * @brief Get last applied direction
 */
motor_direction_t motor_get_direction(motor_id_t motor);


#endif /* MOTOR_DRIVER_H */
