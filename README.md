# MINI-SUMO-ROBOT
# 🤖 Mini Sumo Robot

An autonomous Mini Sumo Robot designed for robotics competitions. The robot uses an ESP32 microcontroller, VL53L0X Time-of-Flight sensors, QTR-8A reflectance sensor array and dual DC geared motors to detect opponents, avoid arena boundaries and execute intelligent attacking strategies.

## 🚀 Project Overview

The main objective of this project is to develop an autonomous Mini Sumo Robot capable of detecting opponents, making real-time decisions and pushing opponents outside the competition arena while staying within the arena boundaries.

The robot uses multiple sensors and a differential-drive mechanism for movement and navigation.

## 🔧 Hardware Components

| Component | Quantity | Purpose |
|---|---:|---|
| ESP32 DevKit V1 (30-pin) | 1 | Main microcontroller |
| VL53L0X ToF Sensor | 4 | Opponent detection |
| TB6612FNG Motor Driver | 1 | DC motor control |
| N20 DC Geared Motors | 2 | Robot movement |
| LM2596 Buck Converter | 2 | Voltage regulation |
| QTR-8A Reflectance Sensor | 1 | White boundary detection |
| DIP Switch | 1 | Mode configuration |
| Toggle Switch | 1 | Main power ON/OFF |
| XT30 Connector | 1 | Battery connection |
| Battery Pack | 1 | Power supply |

## ⚙️ Working Principle

### 1. ESP32 Microcontroller
ESP32 DevKit V1 acts as the brain of the robot. It processes sensor data and controls motor movement according to programmed logic.

### 2. VL53L0X ToF Sensors
Four VL53L0X sensors are used for opponent detection.

- Measure the distance between the robot and opponent.
- Communicate with ESP32 using I2C.
- Use XSHUT pins for individual sensor initialization and address assignment.
- Help identify the opponent's direction.

### 3. QTR-8A Boundary Detection
The QTR-8A reflectance sensor array detects the white boundary of the Mini Sumo arena.

- Reads surface reflectance.
- Identifies the white boundary against the black arena.
- Helps prevent the robot from moving outside the arena.

### 4. TB6612FNG Motor Driver
The TB6612FNG controls two N20 DC geared motors.

- Controls forward and reverse movement.
- Supports PWM-based speed control.
- Allows independent control of both motors.

### 5. Power Management
Two LM2596 buck converters are included in the PCB design for regulated power distribution.

The XT30 connector provides the battery input, while the main toggle switch controls power.

## 🏆 Winning Tactics and Strategies

The robot is designed around four Mini Sumo strategies:

**1. The Edge Dodge**
The robot detects the opponent's movement and uses quick directional movement to avoid being pushed outside the arena.

**2. The Bull Rush**
The robot performs a high-speed forward attack when the opponent is detected in front.

**3. The Spinner**
The robot rotates in search mode until an opponent is detected, then adjusts its direction to attack.

**4. The Corner Trap**
The robot approaches the opponent at an angle to reduce its escape space and push it towards the arena boundary.

## 💻 Software and Libraries

The project uses Arduino IDE for ESP32 programming.

Libraries:

- `Wire.h` – I2C communication.
- `VL53L0X.h` – VL53L0X ToF sensor.
- `SparkFun_TB6612.h` – TB6612 motor driver control, if using its
