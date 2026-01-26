# Motor Controller


This guild contains the high level guide on what each file should contain

---------




## A. motor_config.h — Configuration Reference


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



---------