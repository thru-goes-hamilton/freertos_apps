#include <stdio.h>
#include <math.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/ledc.h"
#include "driver/pcnt.h"
#include "esp_timer.h"

/* ================= USER CONFIG ================= */

#define MOTOR_IN1          14
#define MOTOR_IN2          27
#define MOTOR_PWM          12

#define ENCODER_A          32
#define ENCODER_B          33

#define PCNT_UNIT          PCNT_UNIT_0

#define TARGET_RPM         100
#define KP_TEST_VALUE      1.10f    // <<< CHANGE BETWEEN RUNS
#define TEST_DURATION_MS   30000
#define PID_PERIOD_MS      20
#define PWM_MAX            180

#define COUNTS_PER_REV     360      // measured manually
#define GEAR_RATIO         1.0f

/* =============================================== */

static volatile int32_t encoder_count = 0;
static volatile int32_t measured_rpm = 0;
static volatile int32_t pwm_cmd = 0;

static int64_t start_time_us;

/* ================= HARDWARE INIT ================= */

static void motor_hw_init(void)
{
    gpio_set_direction(MOTOR_IN1, GPIO_MODE_OUTPUT);
    gpio_set_direction(MOTOR_IN2, GPIO_MODE_OUTPUT);

    gpio_set_level(MOTOR_IN1, 0);
    gpio_set_level(MOTOR_IN2, 0);

    ledc_timer_config_t timer = {
        .speed_mode = LEDC_HIGH_SPEED_MODE,
        .timer_num = LEDC_TIMER_0,
        .duty_resolution = LEDC_TIMER_8_BIT,
        .freq_hz = 10000,
        .clk_cfg = LEDC_AUTO_CLK
    };
    ledc_timer_config(&timer);

    ledc_channel_config_t ch = {
        .speed_mode = LEDC_HIGH_SPEED_MODE,
        .channel = LEDC_CHANNEL_0,
        .timer_sel = LEDC_TIMER_0,
        .gpio_num = MOTOR_PWM,
        .duty = 0
    };
    ledc_channel_config(&ch);
}

static void encoder_hw_init(void)
{
    pcnt_config_t pcnt = {
        .pulse_gpio_num = ENCODER_A,
        .ctrl_gpio_num  = ENCODER_B,
        .channel        = PCNT_CHANNEL_0,
        .unit           = PCNT_UNIT,
        .pos_mode       = PCNT_COUNT_INC,
        .neg_mode       = PCNT_COUNT_DEC,
        .lctrl_mode     = PCNT_MODE_REVERSE,
        .hctrl_mode     = PCNT_MODE_KEEP,
        .counter_h_lim  = 32767,
        .counter_l_lim  = -32768
    };

    pcnt_unit_config(&pcnt);
    pcnt_counter_pause(PCNT_UNIT);
    pcnt_counter_clear(PCNT_UNIT);
    pcnt_counter_resume(PCNT_UNIT);
}

/* ================= TASKS ================= */

void encoder_task(void *arg)
{
    int16_t last_count = 0;
    int64_t last_time = esp_timer_get_time();

    while (1) {
        int16_t count;
        pcnt_get_counter_value(PCNT_UNIT, &count);

        int64_t now = esp_timer_get_time();
        float dt = (now - last_time) / 1e6f;

        int16_t delta = count - last_count;
        last_count = count;
        last_time = now;

        float revs = (float)delta / COUNTS_PER_REV;
        measured_rpm = (int32_t)((revs / dt) * 60.0f / GEAR_RATIO);

        vTaskDelay(pdMS_TO_TICKS(2));
    }
}

void p_controller_task(void *arg)
{
    TickType_t last = xTaskGetTickCount();

    while (1) {
        vTaskDelayUntil(&last, pdMS_TO_TICKS(PID_PERIOD_MS));

        float error = TARGET_RPM - measured_rpm;
        float out = KP_TEST_VALUE * error;

        if (out > PWM_MAX) out = PWM_MAX;
        if (out < -PWM_MAX) out = -PWM_MAX;

        pwm_cmd = (int32_t)out;
    }
}

void motor_output_task(void *arg)
{
    while (1) {
        int pwm = pwm_cmd;

        if (pwm >= 0) {
            gpio_set_level(MOTOR_IN1, 1);
            gpio_set_level(MOTOR_IN2, 0);
        } else {
            gpio_set_level(MOTOR_IN1, 0);
            gpio_set_level(MOTOR_IN2, 1);
            pwm = -pwm;
        }

        ledc_set_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_0, pwm);
        ledc_update_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_0);

        vTaskDelay(pdMS_TO_TICKS(5));
    }
}

void logger_task(void *arg)
{
    printf("time_ms,rpm,kp\n");

    while (1) {
        int64_t now = esp_timer_get_time();
        int32_t elapsed_ms = (now - start_time_us) / 1000;

        if (elapsed_ms >= TEST_DURATION_MS) {
            pwm_cmd = 0;
            printf("TEST_DONE\n");
            vTaskSuspend(NULL);
        }

        printf("%ld,%ld,%.3f\n",
               elapsed_ms,
               measured_rpm,
               KP_TEST_VALUE);

        vTaskDelay(pdMS_TO_TICKS(20));
    }
}

/* ================= app_main ================= */

void app_main(void)
{
    motor_hw_init();
    encoder_hw_init();

    start_time_us = esp_timer_get_time();

    xTaskCreate(encoder_task, "encoder", 4096, NULL, 5, NULL);
    xTaskCreate(p_controller_task, "p_ctrl", 4096, NULL, 4, NULL);
    xTaskCreate(motor_output_task, "motor", 4096, NULL, 3, NULL);
    xTaskCreate(logger_task, "logger", 4096, NULL, 2, NULL);
}

/*
How You Use This (ZN-Correct Procedure)

1. Lift robot (wheels free)

2. Set KP_TEST_VALUE = 0.2

3. Flash firmware

4. Let it run 30 seconds

5. Copy serial output → save as CSV

6. Increase Kp

7. Repeat


How You Decide Ku

Ku = the FIRST Kp where:

- Oscillation amplitude is constant

- Does NOT decay

- Does NOT grow

- Continues for full 30 seconds
*/