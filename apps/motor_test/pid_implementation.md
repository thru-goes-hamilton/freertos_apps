# BR88 Motor Control: PID Implementation Guide

This documentation covers the theory, implementation, and tuning of the Proportional-Integral-Derivative (PID) controller for the BR88 's DC motors driven by ESP32 controller.

## Overview

A closed-loop feedback system will ensure precise motor velocity. Unlike open-loop systems that simply set a PWM value, this implementation monitors encoder feedback to maintain a consistent RPM regardless of battery level or physical terrain.

## Why PID?

- Load Compensation: Automatically increases torque when the robot encounters resistance (e.g., carpets, inclines).

- Synchronized Drive: Ensures both left and right motors spin at identical speeds to maintain a straight path.

- Precision: Minimizes overshoot, reduces ripples and reduces the time taken to reach target velocities.

## Mathematical Logic

The controller calculates an Error ($e$) defined as:

```$$e(t) = \text{Setpoint} - \text{Measured RPM}$$```

It then calculates the output using three distinct components:

- Proportional (P): Produces an output proportional to the current error.

- Integral (I): Accumulates past errors to eliminate steady-state offsets (e.g., friction).

- Derivative (D): Predicts future error by calculating the rate of change, dampening oscillations.

## Ziegler-Nichols (ZN) Tuning Method

The Ziegler-Nichols method is a heuristic mathematical approach used to determine PID constants by finding the "Ultimate" state of the system.

- Step 1: Experimental Discovery

    Set $K_i$ and $K_d$ to 0.

    Gradually increase $K_p$ until the motor speed begins to oscillate with a constant amplitude (stable waves that do not grow or decay).

    Record the following two values:

    - $K_u$ (Ultimate Gain): The value of $K_p$ that caused the stable oscillation.

    - $T_u$ (Ultimate Period): The time (in seconds) between two consecutive oscillation peaks.

- Step 2: Calculation Table

    Use your recorded $K_u$ and $T_u$ values to calculate the parameters based on your desired control type:

    | Control Type | $K_p$ | $K_i$ | $K_d$ |
    |--------------|-------|-------|-------|
    | P Control    | $0.50 K_u$ | — | — |
    | PI Control   | $0.45 K_u$ | $1.2 K_p / T_u$ | — |
    | Classic PID  | $0.60 K_u$ | $2 K_p / T_u$ | $K_p T_u / 8$ |

Note: For many DC motors, the "PI Control" settings often provide the most stable results with the least noise.

## Manual Tuning Procedure (Alternative)

If you prefer an iterative approach without measuring oscillation periods:

1. Preparation

    Place BR88 on a stand so wheels can spin freely.

    Define a constant target speed (e.g., 100 RPM).

    Initialize $K_p$, $K_i$, and $K_d$ to 0.0.

2. Find the Proportional Base ($K_p$)

    Increase $K_p$ in increments of 0.1.

    Stop when the motor begins to oscillate steadily around the target.

    Set $K_p$ to half of this value.

3. Add Dampening ($K_d$)

    Increase $K_d$ to remove the "bounciness."

    Goal: The motor should reach the target speed quickly with minimal overshoot.

    Warning: High $K_d$ values can cause high-frequency noise/vibration.

4. Fix Steady-State Error ($K_i$)

    If the motor settles at 98 RPM when the target is 100, increase $K_i$ in very small increments (0.01).

    Goal: This forces the error to zero over time.

    Troubleshooting Cheat Sheet

    | Symptom | Adjustment |
    |---------|------------|
    | Slow response to speed changes | Increase $K_p$ |
    | Overshoots the target speed | Increase $K_d$ |
    | Never reaches target (offset) | Increase $K_i$ |
    | Vibrating or jittery motors | Decrease $K_d$ |
    | Wild oscillations (hunting) | Decrease $K_p$ or $K_i$ |

## Safety & Best Practices

1. Anti-Windup

    To prevent the Integral term from growing to infinity if the wheels are physically stalled, we implement an Integral Limit:

    ```
    // Example Anti-Windup Logic
    if (output >= MAX_PWM || output <= MIN_PWM) {
        // Stop accumulating integral error
    } else {
        integral += error * dt;
    }
    ```

2. Loop Timing

    PID math depends on time ($dt$). Ensure your control task runs at a consistent frequency (e.g., 50Hz / 20ms) using a dedicated FreeRTOS task or hardware timer.