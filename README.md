# Hands-Free Candy/Toy Machine

A contactless candy or toy dispensing machine built with Arduino. Uses a two-board parent-child architecture with servo motor control, LED lighting effects, and sound.

## Overview

This project creates an interactive vending experience where users can dispense candy or small toys without physical contact. The parent board handles motor control and sensing, while the child board manages lights and audio feedback.

## Hardware Requirements

### Parent Board
- Arduino (Uno or compatible)
- Continuous rotation servo motor
- Push button (dispense trigger)
- Reverse button (for clearing jams)
- IR break-beam sensor or similar (detects dispensed items)

### Child Board
- Arduino (Uno or compatible)
- NeoPixel LED strip (30 LEDs)
- Adafruit Audio FX Sound Board

### Wiring

**Parent Board Pins:**
| Component | Pin |
|-----------|-----|
| Dispense Button | 2 |
| Sensor | 4 |
| Reverse Button | 7 |
| Servo | 10 |

**Child Board Pins:**
| Component | Pin |
|-----------|-----|
| Sound Board RST | 4 |
| Sound Board TX | 5 |
| Sound Board RX | 6 |
| NeoPixel Data | 8 |
| Sound Board ACT | 10 |

**I2C Connection:**
- Connect SDA and SCL between both boards
- Child board address: 9

## Features

### Dispensing
- Button-triggered dispensing with automatic stop when sensor detects item
- 6-second cooldown between dispenses to prevent rapid triggering
- Manual stop capability after 5 seconds of holding button
- Reverse mode for clearing jams

### LED Effects
- **Idle:** Rainbow color cycle
- **Dispensing:** Blue theater chase
- **Dispensed:** Green color wipe
- **Reverse:** Red color wipe

### Audio
- Ambient background music during idle
- Random sound effects when dispensing

## Audio Files

Place these files on the Adafruit Sound Board SD card:
- `AMB02.OGG`, `AMB04.OGG` - Ambient/background music
- `FX01.OGG` through `FX08.OGG` - Dispensing sound effects

## Dependencies

Install these libraries via Arduino Library Manager:
- `Wire` (built-in)
- `Servo` (built-in)
- `Adafruit_NeoPixel`
- `SoftwareSerial` (built-in)
- `Adafruit_Soundboard`

## Installation

1. Install the required Arduino libraries
2. Upload `parent/handsfree_toy_machine_parent.ino` to the parent Arduino
3. Upload `child/handsfree_toy_machine_child.ino` to the child Arduino
4. Load audio files onto the Sound Board SD card
5. Wire components according to the pin tables above
6. Connect both boards via I2C (SDA to SDA, SCL to SCL)

## Configuration

Key constants in the parent sketch:
- `COOLDOWN_MS` (6000): Time between dispenses
- `MANUAL_COOLDOWN_MS` (5000): Hold time for manual stop
- `servoRotate` (115): Forward rotation speed
- `servoReverseRotate` (80): Reverse rotation speed

## License

MIT License - See [LICENSE](LICENSE) for details.
