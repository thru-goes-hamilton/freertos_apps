# PID Motor Control Architecture & Ziegler–Nichols Tuning Guide

Platform: ESP32 + FreeRTOS
Application: BR 88

## Purpose of This Document

This document defines the architecture, implementation steps, and tuning procedure required to convert the existing open-loop motor controller into a closed-loop PID-controlled system using the Ziegler–Nichols (ZN) tuning method.

It is written specifically for:

ESP32 (LEDC PWM + GPIO)

FreeRTOS-based control loop

DC motors with encoder feedback

RC car / mobile robot use cases

## Current System Status (Baseline)

The existing motor controller:

- Converts commanded RPM → PWM duty

- Uses no feedback

- Is open-loop

- Cannot compensate for:

    - Load changes

    - Battery voltage drop

    - Motor mismatch

Therefore, PID control must be added as a new layer, not patched into the existing PWM logic.

## Target Control Architecture

The system must be restructured into three logical layers:

```
Target RPM
    ↓
PID Controller (Closed Loop)
    ↓
Motor Driver (PWM + Direction)
```

## Key Principle

PID never talks directly to hardware

Motor driver never performs control logic

## File & Module Structure

Recommended structure:
```
main
 ├── motor_driver.c       // GPIO + LEDC + direction logic
 ├── motor_driver.h
 ├── pid_motor.c          // PID logic (this document defines it)
 ├── pid_motor.h
 ├── encoder.c            // RPM measurement
 ├── encoder.h
 └── app_main.c
```

### Encoder Feedback Requirement

PID control requires real-time RPM feedback.

Each motor must provide:

```float motor_rpm_measured;```

Feedback may be implemented using:

```
ESP32 PCNT unit

GPIO interrupt counting

Hardware encoder interface
```

Without encoder feedback, PID and ZN tuning are impossible.

## PID Data Structure

Each motor must maintain its own PID state.
```
typedef struct {
    float kp;
    float ki;
    float kd;

    float integral;
    float prev_error;

    float output;
} pid_t;
```

Create one instance per motor:
```
pid_t pid_motor1;
pid_t pid_motor2;
```

## PID Compute Function

The PID controller must run at a fixed interval.
```
float pid_compute(pid_t *pid,
                  float setpoint_rpm,
                  float measured_rpm,
                  float dt)
{
    float error = setpoint_rpm - measured_rpm;

    pid->integral += error * dt;

    // Anti-windup clamp
    if (pid->integral > 1000) pid->integral = 1000;
    if (pid->integral < -1000) pid->integral = -1000;

    float derivative = (error - pid->prev_error) / dt;

    pid->output =
        pid->kp * error +
        pid->ki * pid->integral +
        pid->kd * derivative;

    pid->prev_error = error;

    return pid->output;
}
```

Output unit: RPM command
PWM conversion is handled separately

## PWM Application Layer

The PID output is converted into PWM using the existing motor driver logic:

```set_motor(motor_id, (int32_t)pid_output_rpm);```


This preserves:

- Direction handling

- PWM saturation

- Hardware abstraction

## FreeRTOS PID Control Task

PID must execute at a constant frequency.

Recommended:
```
50 Hz (20 ms loop)

#define PID_PERIOD_MS 20
#define PID_DT (PID_PERIOD_MS / 1000.0f)

void pid_task(void *arg)
{
    while (1) {
        float rpm1 = motor1_rpm_measured;
        float rpm2 = motor2_rpm_measured;

        float out1 = pid_compute(&pid_motor1, target_rpm, rpm1, PID_DT);
        float out2 = pid_compute(&pid_motor2, target_rpm, rpm2, PID_DT);

        set_motor(1, (int32_t)out1);
        set_motor(2, (int32_t)out2);

        vTaskDelay(pdMS_TO_TICKS(PID_PERIOD_MS));
    }
}
```

Never run PID inside a tight while(1) loop without delay.

## Ziegler–Nichols Tuning Procedure

- Step 1: Disable Integral and Derivative
    ```
    pid.ki = 0;
    pid.kd = 0;
    ```

- Step 2: Find Ultimate Gain (Ku)

    1. Gradually increase kp

    2. Observe RPM response

    3. Stop when constant-amplitude oscillation appears

    4. Record:
    ```
    Ku → kp value at oscillation

    Tu → Time between oscillation peaks (seconds)
    ```

## Ziegler–Nichols Gain Calculation
Recommended for DC Motors: PI Control
```
Parameter	Formula
Kp	0.45 × Ku
Ki	1.2 × Kp / Tu
Kd	0

Apply values:

pid.kp = Kp;
pid.ki = Ki;
pid.kd = 0;
```

Repeat tuning individually for each motor.

## Removal of Open-Loop Logic

Once PID is active:

- Remove RPM sweep logic

- Do not manually ramp PWM

- Only set target_rpm

PID handles:

- Acceleration

- Load compensation

- Speed regulation

## BR 88 Specific Enhancements
1. Dead-Zone Compensation

    DC motors do not move at low PWM.
    ```
    if (abs(pid_output) < 20 && target_rpm != 0)
        pid_output = copysignf(20, pid_output);
    ```

2. Output Saturation
```
Clamp output RPM:

±NOLOAD_RPM
```
3. Expected System Behavior

A correctly tuned system will:

- Reach target RPM in < 300 ms

- Maintain speed under load

- Keep both motors synchronized

- Remain stable across battery voltage changes

- Exhibit no audible oscillation

## Summary

To implement PID motor control using Ziegler–Nichols:

- Separate control logic from motor driver

- Add encoder RPM feedback

- Implement motor PID state

- Run PID at fixed time intervals

- Find Ku and Tu experimentally

- Compute gains using ZN table

- Apply PI control with anti-windup

- Remove open-loop PWM logic

- Add dead-zone and saturation handling