# Motor Controller Architecture (ESP32 + Encoder + PID)

## Overview

This motor controller is designed for a differential-drive robot using **GM25 DC gear motors**, an **ESP32**, and **quadrature encoders** mounted on the **motor shaft**.

The high-level idea is simple:

* **ROS (on the Pi)** decides how fast each wheel should rotate
* **ESP32** makes sure the motors actually rotate at that speed
* This is done using **encoder feedback** and a **PID control loop**

Once a speed command reaches the ESP32, everything happens locally and in real time.

---

## Responsibility split

### Raspberry Pi / ROS

* Sends **target RPM** for left and right motors
* Does not care about PWM, voltage, braking, or encoders
* Treats the ESP32 as a low-level motor controller

ROS → *“Left wheel: 120 RPM, Right wheel: 115 RPM”*

---

### ESP32

The ESP32 is responsible for **all motor control details**:

* Reading encoder pulses
* Calculating actual motor RPM
* Running the PID control loop
* Driving PWM and direction pins
* Handling stopping and braking safely

Once RPM values are received, the ESP32 operates fully on its own.

---

## Motor and encoder setup

* Motor: **GM25 DC gear motor (~260 RPM no-load)**
* Encoder: **Quadrature encoder (A/B)**
* Encoder position: **Motor shaft**
* Decoder mode: **2× decoding (initially)**

Encoder pulses are counted using the ESP32 **PCNT hardware**, not GPIO interrupts.
This keeps CPU usage low and timing consistent.

---

## Control loop timing

* The control loop runs at a **fixed rate of 50 Hz**
* That means:

  * Encoder values are processed every **20 ms**
  * PID runs every **20 ms**
  * Motor outputs are updated every **20 ms**

Encoder pulses are counted continuously in hardware, but **read and reset once per loop**.

This timing is well suited for:

* The GM25 motor speed range
* Gearbox inertia
* Encoder resolution

---

## Data flow (one motor)

This is what happens every control cycle:

1. **Target RPM** is already stored (from ROS)
2. **Encoder count** from the last 20 ms is read
3. Encoder count is converted into **measured RPM**
4. PID compares:

   * Target RPM
   * Measured RPM
5. PID computes a corrected RPM command
6. RPM command is converted into:

   * PWM duty
   * Direction pins
7. Motor speed changes
8. Encoder sees the change in the next cycle

This loop repeats continuously.

---

## PID controller role

PID is used to keep motor speed stable even when:

* Load changes
* Battery voltage drops
* Motors are slightly mismatched

### What PID actually does here

* **P (Proportional):** reacts to speed error
* **I (Integral):** removes steady-state offset caused by friction or load
* **D (Derivative):** usually disabled (encoder noise + gearbox make D noisy)

In practice, this system uses **PI control**, not full PID.

---

## Error deadband (important detail)

Encoders naturally produce small noise and jitter, especially near steady speed.

To avoid unnecessary corrections:

* A **small RPM error band** is used
* If the speed error is very small, it is treated as zero

Example:

* Target = 100 RPM
* Measured = 98–102 RPM
* PID treats this as “good enough”

This:

* Reduces jitter
* Keeps motors quiet
* Improves low-speed behavior

The PID loop **still runs every cycle** — only tiny errors are ignored.

---

## Why feedback is never skipped

Even when the error is small:

* Encoder is still read
* PID still runs
* Timing stays constant

We **do not skip updates or freeze feedback**, because:

* PID math depends on consistent timing
* Skipping updates causes drift and instability

Filtering is applied to the **error**, not the control loop itself.

---

## Motor output stage

PID does not directly control PWM.

Instead:

* PID outputs a **commanded RPM**
* That RPM is converted into PWM using a known mapping
* Direction pins handle forward/reverse

This separation keeps:

* Control logic clean
* Hardware handling simple
* Debugging easier

---

## Stop and braking behavior

When ROS sends `RPM = 0`:

1. PID smoothly reduces motor speed
2. Once measured RPM is near zero:

   * Active braking is enabled
3. Motor stops firmly without overshoot

This avoids:

* Sudden current spikes
* Mechanical shock
* PID instability

The motor is not allowed to coast freely.

---

## Multi-motor handling

* Each motor has its **own encoder**
* Each motor has its **own PID state**
* Gains can be tuned independently

This is important because:

* No two motors behave exactly the same
* Gearboxes introduce small differences

---

## Why this architecture works well

This design:

* Keeps speed stable under load
* Makes left and right motors match closely
* Scales well with ROS control
* Is easy to tune using Ziegler–Nichols
* Uses ESP32 hardware features efficiently

Most importantly, it separates:

* **Decision making (ROS)**
* **Real-time control (ESP32)**

---

## One-line summary

**ROS tells the ESP32 what speed it wants, and the ESP32 continuously adjusts motor power using encoder feedback so the motors actually run at that speed.**

---

