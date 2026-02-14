# PCNT Implementation Architecture  
## ESP32 Quadrature Encoder Interface  
Motor: GM25-370CA-12560-21-EN  
CPR: 924 counts per output shaft revolution  

---

# 1. Architecture Overview

The encoder interface uses the ESP32 PCNT (Pulse Counter) peripheral to:

- Decode quadrature A/B signals
- Track direction automatically
- Accumulate position counts
- Provide delta counts for RPM calculation

The system is structured as follows:

Encoder Signals (A/B)
        ↓
GPIO Input
        ↓
ESP32 PCNT Hardware
        ↓
Encoder Driver Layer (encoder_pcnt.c)
        ↓
Speed Calculation (RPM)
        ↓
PID Controller

---

# 2. Why Use PCNT Instead of GPIO Interrupts

Using GPIO interrupts for quadrature decoding:

- Increases CPU load
- Risks missed pulses at higher speeds
- Introduces jitter

PCNT advantages:

- Hardware-based counting
- Direction decoding handled internally
- Minimal CPU overhead
- Stable at several MHz input frequencies

For motor control, PCNT is the correct engineering choice.

---

# 3. ESP32 PCNT Configuration Strategy

## 3.1 PCNT Unit Selection

Use:

PCNT_UNIT_0

One motor → one PCNT unit.

---

## 3.2 Channel Configuration

Quadrature requires two channels:

| PCNT Channel | Purpose |
|--------------|---------|
| Channel 0 | Pulse input (Signal A) |
| Channel 1 | Control input (Signal B) |

Configuration logic:

- Channel A counts pulses.
- Channel B controls increment/decrement direction.

---

## 3.3 Counting Mode

We configure:

- Positive edge increment
- Negative edge increment
- Control signal inverts direction

This achieves 4x quadrature decoding.

---

# 4. Count Limits and Overflow Handling

PCNT counter is 16-bit signed:

Range:

-32768 to +32767

With max pulse rate ≈ 2708 counts/sec:

Overflow occurs in:

32767 / 2708 ≈ 12 seconds

Therefore:

We must periodically read and clear the counter.

Recommended sampling:

10 ms – 50 ms

This completely avoids overflow.

---

# 5. Encoder Driver Layer Design

File Structure:

drivers/
|--encoder_pcnt.h
|--encoder_pcnt.c


---

# 6. Public API Design

The encoder driver should expose:

```c
void encoder_init(void);

int32_t encoder_get_count(void);

int32_t encoder_get_delta(void);

float encoder_get_rpm(float sample_time_sec);

void encoder_reset(void);
```

## 6.1 encoder_init()

Responsibilities:

- Configure GPIO pins
- Configure PCNT unit
- Set filter value
- Set counter limits
- Enable PCNT

## 6.2 encoder_get_count()

Returns cumulative position count.

Used for:

- Position control
- State estimation

## 6.3 6.3 encoder_get_delta()

Returns change in counts since last call.

Used for:
- Speed calculation

Implementation logic:
- Read PCNT value
- Clear PCNT counter
- Return delta

## 6.4 encoder_get_rpm(sample_time)

Implements:
```
RPM = (delta_count × 60) / (CPR × sample_time)

Example:

If sample_time = 0.01 s (10 ms):

RPM = (delta × 60) / (924 × 0.01)
```

---


# 7. Sampling Strategy for Control Loop

Recommended control loop frequency:

100 Hz (10 ms period)

### Why?

- Good balance between responsiveness and noise
- Safe relative to 2.7 kHz pulse rate
- Suitable for PID speed control

### Control task flow:

- Read delta count
- Compute RPM
- Run PID
- Update PWM

---

# 8. Direction Handling

PCNT handles direction internally.

If motor reverses:

- Counter becomes negative

- RPM becomes negative

- PID should accept signed RPM.

---

# 9. Noise Immunity

To ensure clean readings:

- Enable PCNT glitch filter

- Keep encoder wires short

- Use proper grounding

- Avoid routing near motor power wires

## Filter value:

100 (recommended)

---



# 10. Position Tracking

If full position tracking is needed:

Maintain a 32-bit software accumulator:
```
total_position += delta_count;
```
This prevents 16-bit overflow issues.


# 11. Data Flow Summary

```
Encoder Hardware → PCNT → Encoder Driver → Control Task → PID → PWM Driver → Motor
```

Each layer has a single responsibility:

- PCNT: Count pulses

- Encoder driver: Convert counts to meaningful units

- Control task: Apply control law

- Motor driver: Generate PWM


# 12. Real-Time Considerations

Avoid:

- Long blocking calls inside encoder functions

- Floating point inside ISR

-  Reading PCNT from multiple tasks

Best practice:

- Only control task reads encoder.


# 13. Safety Considerations

Encoder failure detection strategy:
```
If:

delta_count == 0
AND
PWM > threshold

for more than X milliseconds

→ trigger fault state
```
This detects:

- Disconnected encoder

- Stalled motor

- Broken wiring