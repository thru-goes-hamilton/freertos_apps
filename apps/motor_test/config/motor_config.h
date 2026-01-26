#ifndef MOTOR_CONFIG_H
#define MOTOR_CONFIG_H

/* =========================================================
 * Motor Controller ECU Configuration
 * ---------------------------------------------------------
 * This file contains all compile-time configuration values
 * for the motor controller subsystem.
 *
 * - No logic
 * - No function definitions
 * - Only macros, constants, and enums
 *
 * ========================================================= */


/* =========================================================
 * 1. Motor Physical Parameters
 * ========================================================= */

// Maximum forward RPM (no-load or rated)
#define MOTOR_MAX_RPM                /* TODO */

// Maximum reverse RPM
#define MOTOR_MAX_REVERSE_RPM        /* TODO */

// RPM treated as zero (deadband)
#define MOTOR_DEADBAND_RPM           /* TODO */


/* =========================================================
 * 2. PWM (LEDC) Configuration
 * ========================================================= */

// PWM frequency in Hz
#define MOTOR_PWM_FREQUENCY_HZ       /* TODO */

// LEDC resolution in bits (e.g., 8 → 0–255)
#define MOTOR_PWM_RESOLUTION_BITS    /* TODO */

// Derived PWM limits
#define MOTOR_PWM_MAX_DUTY           /* TODO */
#define MOTOR_PWM_MIN_DUTY           /* TODO */


/* =========================================================
 * 3. Motor GPIO Pin Mapping
 * ========================================================= */

// Motor 1 pins
#define M1_IN1_GPIO                  /* TODO */
#define M1_IN2_GPIO                  /* TODO */
#define M1_PWM_GPIO                  /* TODO */

// Motor 2 pins
#define M2_IN1_GPIO                  /* TODO */
#define M2_IN2_GPIO                  /* TODO */
#define M2_PWM_GPIO                  /* TODO */


/* =========================================================
 * 4. Encoder Configuration (PCNT)
 * ========================================================= */

// Pulses per revolution (measured manually)
#define ENCODER_PULSES_PER_REV       /* TODO */

// Encoder decoding factor (1x / 2x / 4x)
#define ENCODER_DECODING_FACTOR      /* TODO */

// Total counts per revolution
#define ENCODER_COUNTS_PER_REV       (ENCODER_PULSES_PER_REV * ENCODER_DECODING_FACTOR)


/* =========================================================
 * 5. Encoder Sampling & PCNT Limits
 * ========================================================= */

// Encoder sampling period (ms)
#define ENCODER_SAMPLE_PERIOD_MS     /* TODO */

// PCNT counter limits
#define PCNT_COUNT_HIGH_LIMIT        /* TODO */
#define PCNT_COUNT_LOW_LIMIT         /* TODO */


/* =========================================================
 * 6. Control Loop Timing
 * ========================================================= */

// Control loop period in milliseconds
#define CONTROL_LOOP_PERIOD_MS       /* TODO */

// Control loop frequency (Hz)
#define CONTROL_LOOP_HZ              /* TODO */


/* =========================================================
 * 7. Safety & Watchdog Parameters
 * ========================================================= */

// UART command timeout from Raspberry Pi (ms)
#define UART_COMMAND_TIMEOUT_MS      /* TODO */

// Encoder stall detection timeout (ms)
#define ENCODER_STALL_TIMEOUT_MS     /* TODO */

// RPM threshold for overspeed fault
#define RPM_OVERSPEED_THRESHOLD      /* TODO */


/* =========================================================
 * 8. System Defaults
 * ========================================================= */

// Default RPM on startup
#define DEFAULT_TARGET_RPM           /* TODO */


/* =========================================================
 * 9. System State Definitions
 * ========================================================= */

typedef enum
{
    SYS_STATE_STOP = 0,
    SYS_STATE_RUN,
    SYS_STATE_FAULT
} system_state_t;


#endif /* MOTOR_CONFIG_H */
