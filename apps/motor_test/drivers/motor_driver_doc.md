## motor_driver.[c/h] — Motor Driver Layer (L298N + ESP32)

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
