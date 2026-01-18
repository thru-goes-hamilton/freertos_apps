#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <rcl/rcl.h>
#include <rcl/error_handling.h>
#include <rclc/rclc.h>
#include <rclc/executor.h>
#include <std_msgs/msg/int32_multi_array.h>
#include "driver/gpio.h"
#include "driver/ledc.h"
#include <stdio.h>
#include <stdlib.h> // for abs()

#define RCCHECK(fn) { rcl_ret_t rc = (fn); if(rc != RCL_RET_OK){ printf("Error %d at %d\n", rc, __LINE__); vTaskDelete(NULL); } }
#define RCSOFTCHECK(fn){ rcl_ret_t rc = (fn); if(rc != RCL_RET_OK){ printf("Soft error %d at %d\n", rc, __LINE__); } }

// Motor driver pins
#define M1_IN1 14
#define M1_IN2 27
#define M1_PWM 12 // LEDC_CHANNEL_0
#define M2_IN1 26
#define M2_IN2 25
#define M2_PWM 33 // LEDC_CHANNEL_1

// Reference no-load RPM for 100% duty
#define NOLOAD_RPM 260 // integer

// ROS subscription objects
rcl_subscription_t subscriber;
std_msgs__msg__Int32MultiArray msg;

// Convert commanded integer RPM → 8-bit LEDC duty ticks (0–255)
static uint32_t rpm_to_ticks_int(int32_t rpm_cmd) {

    uint32_t abs_rpm = (uint32_t)( rpm_cmd < 0 ? -rpm_cmd : rpm_cmd );

    uint32_t ticks = (abs_rpm * 255U) / NOLOAD_RPM;

    return ticks > 255U ? 255U : ticks;
}

void subscription_callback(const void * msgin) {

    const std_msgs__msg__Int32MultiArray * m = (const std_msgs__msg__Int32MultiArray *)msgin;

    if (m->data.size < 2) {
        printf("Warning: insufficient data\n");
        return;
    }

    int32_t rpm_l = m->data.data[0];
    int32_t rpm_r = m->data.data[1];

    // Left motor
    uint32_t ticks_l = rpm_to_ticks_int(rpm_l);

    if (rpm_l >= 0) {
        gpio_set_level(M1_IN1, 1);
        gpio_set_level(M1_IN2, 0);
    } else {
        gpio_set_level(M1_IN1, 0);
        gpio_set_level(M1_IN2, 1);
    }

    ledc_set_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_0, ticks_l);
    ledc_update_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_0);

    // Right motor
    uint32_t ticks_r = rpm_to_ticks_int(rpm_r);

    if (rpm_r >= 0) {
        gpio_set_level(M2_IN1, 1);
        gpio_set_level(M2_IN2, 0);
    } else {
        gpio_set_level(M2_IN1, 0);
        gpio_set_level(M2_IN2, 1);
    }

    ledc_set_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_1, ticks_r);
    ledc_update_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_1);
}

void appMain(void * arg) {

    printf("\n*** Serial test: motor_controller starting! ***\n\n");
    // GPIO direction
    gpio_set_direction(M1_IN1, GPIO_MODE_OUTPUT);
    gpio_set_direction(M1_IN2, GPIO_MODE_OUTPUT);
    gpio_set_direction(M2_IN1, GPIO_MODE_OUTPUT);
    gpio_set_direction(M2_IN2, GPIO_MODE_OUTPUT);
    // stop motors
    gpio_set_level(M1_IN1, 0); gpio_set_level(M1_IN2, 0);
    gpio_set_level(M2_IN1, 0); gpio_set_level(M2_IN2, 0);

    // LEDC timer (8-bit)
    ledc_timer_config_t tcfg = {
        .speed_mode = LEDC_HIGH_SPEED_MODE,
        .timer_num = LEDC_TIMER_0,
        .duty_resolution = LEDC_TIMER_8_BIT,
        .freq_hz = 10000,
        .clk_cfg = LEDC_AUTO_CLK
    };
    ledc_timer_config(&tcfg);

    // LEDC channels
    ledc_channel_config_t ccfg = {
        .speed_mode = LEDC_HIGH_SPEED_MODE,
        .timer_sel = LEDC_TIMER_0,
        .duty = 0,
        .hpoint = 0
    };
    ccfg.channel = LEDC_CHANNEL_0; ccfg.gpio_num = M1_PWM; ledc_channel_config(&ccfg);
    ccfg.channel = LEDC_CHANNEL_1; ccfg.gpio_num = M2_PWM; ledc_channel_config(&ccfg);

    msg.data.capacity = 2;
    msg.data.size = 0;
    msg.data.data = malloc(sizeof(int32_t) * 2);

    // ROS2 init
    rcl_allocator_t allocator = rcl_get_default_allocator();

    rclc_support_t support;
    RCCHECK(rclc_support_init(&support, 0, NULL, &allocator));

    rcl_node_t node;
    RCCHECK(rclc_node_init_default(&node, "motor_controller", "", &support));

    RCCHECK(rclc_subscription_init_default(
    &subscriber, &node,
    ROSIDL_GET_MSG_TYPE_SUPPORT(std_msgs, msg, Int32MultiArray),
    "/marl_bot2/rpm"
    ));
    
    rclc_executor_t executor;
    RCCHECK(rclc_executor_init(&executor, &support.context, 1, &allocator));
    RCCHECK(rclc_executor_add_subscription(&executor, &subscriber, &msg, &subscription_callback, ON_NEW_DATA));

    printf("Motor controller initalised\n");

    while (1) {
        rclc_executor_spin_some(&executor, RCL_MS_TO_NS(100));
        vTaskDelay(pdMS_TO_TICKS(100));
    } 
}
