#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/ledc.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

// Motor driver pins
#define M1_IN1 14
#define M1_IN2 27
#define M1_PWM 12
#define M2_IN1 26
#define M2_IN2 25
#define M2_PWM 33

#define NOLOAD_RPM 260  // Reference RPM at full speed
#define SWEEP_DURATION_MS 5000  // Time to go from 0 to full RPM (in ms)
#define HOLD_DURATION_MS 3000   // Hold full RPM
#define STEP_DELAY_MS 100       // Time between RPM steps

// Convert commanded RPM → 8-bit LEDC duty (0–255)
uint32_t rpm_to_ticks_int(int32_t rpm_cmd) {
    uint32_t abs_rpm = (uint32_t)(rpm_cmd < 0 ? -rpm_cmd : rpm_cmd);
    uint32_t ticks = (abs_rpm * 255U) / NOLOAD_RPM;
    return ticks > 255U ? 255U : ticks;
}

void set_motor(int motor_id, int32_t rpm_cmd) {
    int in1, in2, pwm_ch;
    if (motor_id == 1) {
        in1 = M1_IN1; in2 = M1_IN2; pwm_ch = LEDC_CHANNEL_0;
    } else {
        in1 = M2_IN1; in2 = M2_IN2; pwm_ch = LEDC_CHANNEL_1;
    }

    uint32_t ticks = rpm_to_ticks_int(rpm_cmd);
    if (rpm_cmd >= 0) {
        gpio_set_level(in1, 1);
        gpio_set_level(in2, 0);
    } else {
        gpio_set_level(in1, 0);
        gpio_set_level(in2, 1);
    }
    ledc_set_duty(LEDC_HIGH_SPEED_MODE, pwm_ch, ticks);
    ledc_update_duty(LEDC_HIGH_SPEED_MODE, pwm_ch);
}

void app_main(void) {
    // Set motor GPIO direction
    gpio_set_direction(M1_IN1, GPIO_MODE_OUTPUT);
    gpio_set_direction(M1_IN2, GPIO_MODE_OUTPUT);
    gpio_set_direction(M2_IN1, GPIO_MODE_OUTPUT);
    gpio_set_direction(M2_IN2, GPIO_MODE_OUTPUT);

    // Stop motors
    gpio_set_level(M1_IN1, 0); gpio_set_level(M1_IN2, 0);
    gpio_set_level(M2_IN1, 0); gpio_set_level(M2_IN2, 0);

    // LEDC config
    ledc_timer_config_t tcfg = {
        .speed_mode = LEDC_HIGH_SPEED_MODE,
        .timer_num = LEDC_TIMER_0,
        .duty_resolution = LEDC_TIMER_8_BIT,
        .freq_hz = 10000,
        .clk_cfg = LEDC_AUTO_CLK
    };
    ledc_timer_config(&tcfg);

    ledc_channel_config_t ccfg = {
        .speed_mode = LEDC_HIGH_SPEED_MODE,
        .timer_sel = LEDC_TIMER_0,
        .duty = 0,
        .hpoint = 0
    };
    ccfg.channel = LEDC_CHANNEL_0; ccfg.gpio_num = M1_PWM; ledc_channel_config(&ccfg);
    ccfg.channel = LEDC_CHANNEL_1; ccfg.gpio_num = M2_PWM; ledc_channel_config(&ccfg);

    printf("Motor sweep test started...\n");

    while (1) {
        // Sweep forward
        for (int t = 0; t <= SWEEP_DURATION_MS; t += STEP_DELAY_MS) {
            int32_t rpm = (NOLOAD_RPM * t) / SWEEP_DURATION_MS;
            printf("Time: %d ms | RPM: %d\n", t, rpm);
            set_motor(1, rpm);
            set_motor(2, rpm);
            vTaskDelay(pdMS_TO_TICKS(STEP_DELAY_MS));
        }

        // Hold max RPM
        printf("Holding max RPM...\n");
        set_motor(1, NOLOAD_RPM);
        set_motor(2, NOLOAD_RPM);
        vTaskDelay(pdMS_TO_TICKS(HOLD_DURATION_MS));

        // Sweep backward
        for (int t = 0; t <= SWEEP_DURATION_MS; t += STEP_DELAY_MS) {
            int32_t rpm = -(NOLOAD_RPM * t) / SWEEP_DURATION_MS;
            printf("Time: %d ms | RPM: %d\n", t, rpm);
            set_motor(1, rpm);
            set_motor(2, rpm);
            vTaskDelay(pdMS_TO_TICKS(STEP_DELAY_MS));
        }

        // Hold reverse max RPM
        printf("Holding reverse max RPM...\n");
        set_motor(1, -NOLOAD_RPM);
        set_motor(2, -NOLOAD_RPM);
        vTaskDelay(pdMS_TO_TICKS(HOLD_DURATION_MS));

        // Stop before looping
        set_motor(1, 0);
        set_motor(2, 0);
        printf("Loop complete. Restarting...\n\n");
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
