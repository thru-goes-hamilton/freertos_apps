# Motor Controller


This contains the high level guide on what each file should contain


## File Structure

```

motor_test/
├── app.c                     ← Entry point (FreeRTOS init)
│
├── config/
│   ├── motor_config.h        ← Pin map, limits, constants
│   ├── control_config.h      ← Loop rates, PID limits
│
├── drivers/
│   ├── motor_driver.c        ← PWM + direction (L298N)
│   ├── motor_driver.h
│   ├── encoder_pcnt.c        ← Encoder using PCNT
│   ├── encoder_pcnt.h
│
├── control/
│   ├── motor_controller.c    ← High-level motor logic
│   ├── motor_controller.h
│   ├── pid_controller.c      ← PID math (later)
│   ├── pid_controller.h
│
├── comms/
│   ├── uart_protocol.c       ← UART RX/TX + parsing
│   ├── uart_protocol.h
│
├── system/
│   ├── system_state.c        ← RUN / STOP / FAULT
│   ├── system_state.h
│   ├── safety.c              ← watchdog, timeout logic
│   ├── safety.h
│
├── test/
│   ├── pid_tuning.c          ← Ku / ZN experiments (already)
│
motor_test/
├── app.c                     ← Entry point (FreeRTOS init)
│
├── config/
│   ├── motor_config.h        ← Pin map, limits, constants
│   ├── control_config.h      ← Loop rates, PID limits
│
├── drivers/
│   ├── motor_driver.c        ← PWM + direction (L298N)
│   ├── motor_driver.h
│   ├── encoder_pcnt.c        ← Encoder using PCNT
│   ├── encoder_pcnt.h
│
├── control/
│   ├── motor_controller.c    ← High-level motor logic
│   ├── motor_controller.h
│   ├── pid_controller.c      ← PID math (later)
│   ├── pid_controller.h
│
├── comms/
│   ├── uart_protocol.c       ← UART RX/TX + parsing
│   ├── uart_protocol.h
│
├── system/
│   ├── system_state.c        ← RUN / STOP / FAULT
│   ├── system_state.h
│   ├── safety.c              ← watchdog, timeout logic
│   ├── safety.h
│
├── test/
│   ├── pid_tuning.c          ← Ku / ZN experiments (already)
│
├── final_architecture.md


```


Following are the files for motor controller:



---------




## A. *motor_config.h — Configuration Reference*


---

### 1. Motor & Driver Parameters

| Macro | Type | Description | Typical Value |
|-----|-----|------------|---------------|
| `MOTOR_MAX_RPM` | `int` | Maximum forward RPM (no-load) | `260` |
| `MOTOR_MIN_RPM` | `int` | Minimum RPM (stop) | `0` |
| `MOTOR_MAX_REVERSE_RPM` | `int` | Maximum reverse RPM | `-260` |
| `MOTOR_DEADBAND_RPM` | `int` | RPM range treated as zero | `5` |

---

### 2. PWM (LEDC) Configuration

| Macro | Type | Description | Typical Value |
|-----|-----|------------|---------------|
| `MOTOR_PWM_FREQUENCY_HZ` | `int` | PWM frequency for motors | `10000` |
| `MOTOR_PWM_RESOLUTION_BITS` | `int` | LEDC resolution in bits | `8` |
| `MOTOR_PWM_MAX_DUTY` | `int` | Maximum PWM duty | `255` |
| `MOTOR_PWM_MIN_DUTY` | `int` | Minimum PWM duty | `0` |

---

### 3. Motor GPIO Mapping

| Macro | Description | GPIO |
|-----|------------|------|
| `M1_IN1_GPIO` | Motor 1 direction pin IN1 | `14` |
| `M1_IN2_GPIO` | Motor 1 direction pin IN2 | `27` |
| `M1_PWM_GPIO` | Motor 1 PWM output | `12` |
| `M2_IN1_GPIO` | Motor 2 direction pin IN1 | `26` |
| `M2_IN2_GPIO` | Motor 2 direction pin IN2 | `25` |
| `M2_PWM_GPIO` | Motor 2 PWM output | `33` |

---

### 4. Encoder Configuration (PCNT)

| Macro | Type | Description | Typical Value |
|-----|-----|------------|---------------|
| `ENCODER_PULSES_PER_REV` | `int` | Encoder pulses per revolution (measured) | `11` |
| `ENCODER_DECODING_FACTOR` | `int` | Encoder decoding (1x / 2x / 4x) | `2` |
| `ENCODER_COUNTS_PER_REV` | `int` | Total counts per revolution | `ENCODER_PULSES_PER_REV * ENCODER_DECODING_FACTOR` |

---

### 5. Encoder Sampling & Limits

| Macro | Type | Description | Typical Value |
|-----|-----|------------|---------------|
| `ENCODER_SAMPLE_PERIOD_MS` | `int` | Encoder sampling period | `20` |
| `PCNT_COUNT_HIGH_LIMIT` | `int` | PCNT upper limit | `10000` |
| `PCNT_COUNT_LOW_LIMIT` | `int` | PCNT lower limit | `-10000` |

---

### 6. Control Loop Timing

| Macro | Type | Description | Typical Value |
|-----|-----|------------|---------------|
| `CONTROL_LOOP_PERIOD_MS` | `int` | Motor control loop period | `20` |
| `CONTROL_LOOP_HZ` | `int` | Control loop frequency | `50` |

---

### 7. Safety & Watchdog Parameters

| Macro | Type | Description | Typical Value |
|-----|-----|------------|---------------|
| `UART_COMMAND_TIMEOUT_MS` | `int` | Timeout for Pi command | `200` |
| `ENCODER_STALL_TIMEOUT_MS` | `int` | Encoder no-motion timeout | `300` |
| `RPM_OVERSPEED_THRESHOLD` | `int` | RPM overspeed limit | `280` |

---

### 8. System Defaults

| Macro | Type | Description | Typical Value |
|-----|-----|------------|---------------|
| `DEFAULT_TARGET_RPM` | `int` | Startup RPM command | `0` |
| `SYSTEM_STARTUP_STATE` | `enum` | Initial system state | `SYS_STOP` |

---





-------------

## B. *motor_driver.[c/h] — Motor Driver Layer (L298N + ESP32)*

This module provides the **lowest-level motor actuation interface**.
It directly controls GPIO direction pins and PWM output using ESP32 LEDC.

No encoder logic, PID logic, UART logic, or safety decisions are implemented here.

---

### 1. Responsibilities

| Responsibility | Included |
|---------------|----------|
| GPIO direction control | Yes |
| PWM generation (LEDC) | Yes |
| Motor enable / disable | Yes |
| Signed speed handling (fwd/rev) | Yes |
| RPM control logic | No |
| PID control | No |
| Safety decisions | No |
| Communication | No |

---

### 2. Header File: `motor_driver.h`

#### 2.1 Public Data Types

| Type | Description |
|----|------------|
| `motor_id_t` | Identifies left/right motor |
| `motor_dir_t` | Motor direction enum |

---

#### 2.2 Enumerations

| Enum | Values | Purpose |
|----|------|---------|
| `motor_id_t` | `MOTOR_LEFT`, `MOTOR_RIGHT` | Select motor instance |
| `motor_dir_t` | `MOTOR_STOP`, `MOTOR_FORWARD`, `MOTOR_REVERSE` | Direction abstraction |

---

#### 2.3 Public API Functions

| Function | Description | Notes |
|-------|------------|------|
| `motor_driver_init()` | Initializes GPIO and PWM hardware | Called once at boot |
| `motor_set_duty()` | Sets PWM duty for a motor | Duty only, no direction |
| `motor_set_direction()` | Sets motor direction pins | No PWM change |
| `motor_set_output()` | Sets direction + PWM together | Main control API |
| `motor_stop()` | Actively brakes motor | Sets duty = 0 |
| `motor_disable_all()` | Disables all motors | Used in FAULT |

---

### 3. Source File: `motor_driver.c`

#### 3.1 Internal Responsibilities

| Functionality | Description |
|--------------|------------|
| GPIO setup | Configures IN1/IN2 pins as output |
| LEDC setup | Timer + channel initialization |
| Duty update | Updates PWM duty safely |
| Direction switching | Handles sign changes cleanly |

---

#### 3.2 Internal (Static) Functions

| Function | Purpose |
|-------|--------|
| `motor_gpio_init()` | Configure motor direction pins |
| `motor_pwm_init()` | Configure LEDC timers and channels |
| `apply_motor_direction()` | Set IN1/IN2 based on direction |
| `apply_motor_pwm()` | Clamp and write PWM duty |

---

### 4. Expected Inputs & Outputs

| Input | Source |
|-----|-------|
| Motor ID | Motor controller |
| Direction | Motor controller |
| PWM duty | Control logic / PID |

| Output | Destination |
|------|------------|
| GPIO IN1/IN2 | L298N |
| PWM signal | L298N ENA/ENB |

---

### 5. Constraints & Rules

| Rule | Reason |
|----|-------|
| No dynamic memory | Deterministic behavior |
| No blocking calls | Used inside real-time loop |
| No printf/logging | Timing safety |
| No encoder references | Clean separation |

---

### 6. Error Handling Strategy

| Scenario | Action |
|-------|-------|
| Invalid motor ID | Ignore command |
| Duty > max | Clamp to max |
| Duty < min | Clamp to zero |
| Direction change | Apply direction first, then PWM |

---

### 7. Usage Pattern (High Level)

| Step | Description |
|----|------------|
| 1 | Call `motor_driver_init()` at startup |
| 2 | Control task calls `motor_set_output()` |
| 3 | Safety task may call `motor_disable_all()` |
| 4 | No other module touches GPIO/PWM |

---

### 8. Design Rationale

| Design Choice | Reason |
|-------------|--------|
| Direction separated from duty | Prevents shoot-through |
| Signed speed not handled here | Keeps driver generic |
| Header-only config usage | No magic numbers |

---

### 9. Files That Depend on This Module

| File | Usage |
|----|------|
| `motor_controller.c` | Primary consumer |
| `safety.c` | Emergency stop |
| `app.c` | Initialization only |

---



-------------