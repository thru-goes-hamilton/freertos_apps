# Encoder Technical Documentation  
## Motor: GM25-370CA-12560-21-EN  
## Controller: ESP32 (PCNT Based Quadrature Decoding)

---

# 1. Encoder Electrical Information

## Wire Mapping (6 Wires Total)

| Wire Color | Function | Notes |
|------------|----------|-------|
| Red        | Motor +12V | Motor power supply |
| Black      | Motor GND  | Motor ground |
| Green      | Encoder GND | Connect to ESP32 GND |
| Blue       | Encoder VCC | Use 3.3V |
| Yellow     | Channel A | Quadrature signal A |
| White      | Channel B | Quadrature signal B |

## Voltage Rating

- Encoder supports 3.3V – 5V supply.
- ESP32 GPIO pins are NOT 5V tolerant.
- If encoder is powered with 5V, A/B outputs become 5V and can damage ESP32 GPIOs.

### Correct Practice

Power encoder from ESP32 3.3V rail.

---

# 2. Encoder Type

## Type: Magnetic Hall-Effect Quadrature Encoder

### Working Principle

- A multi-pole magnetic disc is mounted on the motor shaft.
- Two Hall sensors generate square waves.
- Channel B is phase shifted by 90 degrees from Channel A.
- Provides:
  - Speed measurement
  - Direction detection
  - Higher resolution using 4x decoding

---

# 3. Motor Shaft PPR (Raw Encoder Resolution)

## Raw Encoder PPR

11 pulses per motor shaft revolution (per channel)

Some generic listings mention 12 PPR, but official TT Motor specification for 370-series 25mm motors is 11 PPR.

Use:

ENCODER_MOTOR_PPR = 11


---

# 4. Gearbox Ratio

Model string:

GM25-370CA-12560-21-EN

The "-21" indicates:

Gear Ratio = 1:21

### Verification

Base motor speed ≈ 3700 RPM

3700 / 21 = 176 RPM

This matches rated output speed of approximately 170–176 RPM.

Use:

ENCODER_GEAR_RATIO = 21


---

# 5. Decoding and Counts Per Revolution (CPR)

Quadrature 4x decoding is used for maximum resolution.

## Formula

CPR = Motor_PPR × Gear_Ratio × 4

## Calculation

11 × 21 × 4 = 924

## Final Value

ENCODER_COUNTS_PER_REV = 924


This means:

924 encoder counts = 1 full revolution of the output shaft.

---

# 6. Maximum Expected RPM and Frequency

## Maximum Output RPM

176 RPM (no-load)

Convert to revolutions per second:

176 / 60 = 2.93 RPS

## Maximum Pulse Frequency

2.93 × 924 = 2708 counts per second

Approximate maximum frequency:

~2.7 kHz

This is well within ESP32 PCNT capability.

---

# 7. PCNT Filter Configuration

ESP32 PCNT uses:

APB Clock = 80 MHz

If filter value = 100:

100 / 80,000,000 = 1.25 microseconds

This rejects pulses shorter than 1.25 µs.

Real pulse period at maximum speed:

1 / 2708 ≈ 369 µs

Therefore, filter value 100–200 is safe.

## Recommended

ENCODER_PCNT_FILTER = 100


---

# 8. Firmware Configuration (motor_config.h)

```c
/* Encoder Configuration */

#define ENCODER_MOTOR_PPR            11
#define ENCODER_GEAR_RATIO           21
#define ENCODER_QUADRATURE_FACTOR    4

#define ENCODER_COUNTS_PER_REV \
    (ENCODER_MOTOR_PPR * ENCODER_GEAR_RATIO * ENCODER_QUADRATURE_FACTOR)

#define ENCODER_MAX_RPM              176
#define ENCODER_PCNT_FILTER          100

```

# 9. RPM Calculation Formula

RPM is calculated using:

```
RPM = (Delta_Count / ENCODER_COUNTS_PER_REV) × (60 / T)


Where:

Delta_Count = encoder counts in sampling window

T = sampling time in seconds
```

Example (10 ms sampling)

T = 0.01 seconds
```
float rpm = (delta_count * 60.0f) /
            (ENCODER_COUNTS_PER_REV * 0.01f);
```

# 10. Direction Detection

Quadrature decoding provides direction information:

|Condition|Direction|
|---|---|
|A leads B|Forward|
|B leads A|Reverse|
ESP32 PCNT hardware handles direction automatically when configured correctly.

# 11. Sumaary

|Parameter|Value|
|---|---|
|Encoder Type|Magnetic Hall-Effect|
|Motor Shaft PPR|11|
|Gearbox Ratio|21 (1:21)|
|Decoding Mode|Quadrature 4x|
|Output CPR|924|
|Max Output RPM|176|
|Max Pulse Frequency|~2.7 kHz|
|Recommended PCNT Filter|100|