# APPS/TPS Plausibility Monitor

Motorsport safety monitor for redundant Accelerator Pedal Position Sensors (APPS) and Throttle Position Sensors (TPS). Implements plausibility checks per FSAE/Formula Student regulations to detect sensor failures and prevent runaway throttle conditions.

## Features

- APPS sensor redundancy check (10% threshold, 100ms fault detection)
- TPS sensor redundancy check (10% threshold, 100ms fault detection)
- APPS-TPS agreement check (10% threshold, 1s before ignition cut)
- Automatic Electronic Throttle Body (ETB) shutdown on fault
- Ignition and fuel system shutdown on severe fault
- Recovery mechanism requiring throttle return to default position
- Real-time ANSI terminal debug output
- Visual LED fault indication

## Safety Requirements Compliance

### Rule 1: APPS Sensor Plausibility
If the difference between APPS main and APPS track sensors exceeds 10% for more than 100ms, the Electronic Throttle Body power must be immediately shut down.

**Implementation:** Continuous monitoring triggers ETB shutdown at 100ms threshold. Fault clears when sensors agree within 10%.

### Rule 2: TPS Sensor Plausibility
If the difference between TPS main and TPS track sensors exceeds 10% for more than 100ms, the Electronic Throttle Body power must be immediately shut down.

**Implementation:** Continuous monitoring triggers ETB shutdown at 100ms threshold. Fault clears when sensors agree within 10%.

### Rule 3: APPS-TPS Agreement
If throttle position differs from expected APPS target by more than 10% for more than 1 second, fuel flow and ignition system must be shut down.

**Implementation:** Immediate ETB shutdown when fault detected. IGN shutdown triggered after 1000ms. Fault clears when APPS and TPS agree within 10%.

### Universal Recovery Condition
All shutdowns remain active until TPS indicates throttle is at or below unpowered default position (≤10%) for 1 second or longer, and all fault conditions have cleared.

## Hardware Requirements

### Microcontroller
- Arduino Uno, Nano, Mega, or compatible board
- Minimum 6 analog input pins
- 5V operating voltage

### Pin Assignments

| Pin | Function | Description |
|-----|----------|-------------|
| D2 | ETB Control | Digital output to electronic throttle relay/MOSFET (HIGH=powered, LOW=shutdown) |
| D7 | IGN Control | Digital output to ignition/fuel relay (HIGH=powered, LOW=shutdown) |
| A4 | TPS Main | Analog input from primary TPS sensor (0-5V) |
| A5 | TPS Track | Analog input from redundant TPS sensor (0-5V, typically inverted) |
| A6 | APPS Main | Analog input from primary APPS sensor (0-5V) |
| A7 | APPS Track | Analog input from redundant APPS sensor (0-5V, typically inverted) |
| LED_BUILTIN | Status LED | Visual fault indication (built-in LED) |

### Wiring Notes

- All sensor inputs expect 0-5V analog signals
- ETB and IGN outputs are digital (5V/0V) and should drive relay coils or MOSFET gates
- Use appropriate current-limiting resistors and flyback diodes for relay circuits
- Ensure proper grounding between Arduino, ECU, and sensors to prevent noise
- Consider using twisted-pair shielded cable for sensor wiring

## Installation

### 1. Clone Repository
```bash
git clone https://github.com/s0Appy/Throttle-Position-Plausibility-System.git
cd Throttle-Position-Plausibility-System
```

### 2. Open in Arduino IDE or PlatformIO

**Arduino IDE:**
```bash
# Open the .ino file
arduino ThrottlePositionPlausibilitySystem.ino
```

**PlatformIO (Recommended for debugging):**
```bash
# Initialize PlatformIO project
pio init --board uno

# Copy .ino file to src/ directory
cp ThrottlePositionPlausibilitySystem.ino src/main.cpp

# Build and upload
pio run --target upload
```

### 3. Configure Sensor Calibration

Open the code and modify these constants to match your sensor voltage ranges:
```cpp
// APPS sensor voltages
const float appsMainVoltageRaw_100 = 4.5;   // Voltage at 100% pedal position
const float appsMainVoltageRaw_0 = 0.5;     // Voltage at 0% pedal position
const float appsTrackVoltageRaw_100 = 0.5;  // Track sensor (inverted)
const float appsTrackVoltageRaw_0 = 4.5;    // Track sensor (inverted)

// TPS sensor voltages
const float tpsMainVoltageRaw_100 = 4.5;    // Voltage at 100% throttle open
const float tpsMainVoltageRaw_0 = 0.5;      // Voltage at 0% throttle closed
const float tpsTrackVoltageRaw_100 = 0.5;   // Track sensor (inverted)
const float tpsTrackVoltageRaw_0 = 4.5;     // Track sensor (inverted)
```

**How to determine your values:**
1. Connect sensors to Arduino
2. Measure voltage with multimeter at 0% and 100% positions
3. Update constants accordingly
4. Track sensors are typically inverted (high voltage = low position)

### 4. Upload to Arduino

**Via Arduino IDE:**
1. Select board: Tools → Board → Arduino Uno (or your board)
2. Select port: Tools → Port → /dev/ttyUSB0 (or your port)
3. Click Upload button

**Via PlatformIO:**
```bash
pio run --target upload
```

## Testing and Debug Setup

### Serial Monitor Requirements

The debug output uses ANSI escape codes for live-updating display. Standard Arduino IDE Serial Monitor does NOT support these codes and will show garbled output.

**Compatible Terminals:**
- PlatformIO Serial Monitor (recommended)
- PuTTY (Windows)
- Tera Term (Windows)
- CoolTerm (cross-platform)
- screen (Linux/Mac)
- minicom (Linux)

**Not Compatible:**
- Arduino IDE Serial Monitor (will show escape codes as text)

### Using PlatformIO Serial Monitor (Recommended)
```bash
# Open serial monitor at 115200 baud
pio device monitor --baud 115200

# Exit with Ctrl+C
```

### Using screen (Linux/Mac)
```bash
# Connect to serial port
screen /dev/ttyUSB0 115200

# Exit with Ctrl+A then K, then Y
```

### Using PuTTY (Windows)

1. Open PuTTY
2. Connection type: Serial
3. Serial line: COM3 (check Device Manager for your port)
4. Speed: 115200
5. Click Open

### Using minicom (Linux)
```bash
# Configure minicom (one time)
sudo minicom -s
# Set serial port to /dev/ttyUSB0
# Set baud rate to 115200
# Save as default

# Run minicom
minicom

# Exit with Ctrl+A then X
```

### Debug Output Format

When connected with a compatible terminal, you'll see:
```
APPS Main V:      4.23
APPS Track V:     0.78
TPS Main V:       2.15
TPS Track V:      2.89
APPS Main %:      82.6
APPS Track %:     80.4
TPS Main %:       36.7
TPS Track %:      38.1
Delta APPS:       2.2
Delta TPS:        1.4
Delta APPS-TPS:   45.9
APPS Fault:       OK
TPS Fault:        OK
APPS-TPS Fault:   ACTIVE
ETB Shutdown:     SHUTDOWN
IGN Shutdown:     OK
Recovery Timer:   IDLE
```

Values update in real-time at approximately 20Hz.

### Bench Testing with Potentiometers

Before vehicle installation, test the system using potentiometers to simulate sensors:

**Required Hardware:**
- 2x 10kΩ potentiometers (for APPS simulation)
- 2x 10kΩ potentiometers (for TPS simulation)
- Breadboard and jumper wires

**Wiring:**
1. Connect potentiometer pins 1 to GND
2. Connect potentiometer pins 3 to 5V
3. Connect potentiometer wipers (pin 2) to analog inputs:
   - Pot 1 → A6 (APPS Main)
   - Pot 2 → A7 (APPS Track)
   - Pot 3 → A4 (TPS Main)
   - Pot 4 → A5 (TPS Track)

**Test Procedure:**

1. **Normal Operation Test**
   - Turn all pots to similar positions
   - Verify outputs stay HIGH
   - LED should be OFF

2. **APPS Fault Test**
   - Turn APPS pots to different positions (>10% difference)
   - Wait 100ms
   - ETB should go LOW
   - LED should turn solid ON
   - Return pots to similar positions
   - Return TPS pots to <10% position
   - Wait 1 second
   - System should recover

3. **TPS Fault Test**
   - Turn TPS pots to different positions (>10% difference)
   - Wait 100ms
   - ETB should go LOW
   - LED should turn solid ON
   - Return pots to similar positions
   - Return TPS pots to <10% position
   - Wait 1 second
   - System should recover

4. **APPS-TPS Mismatch Test**
   - Set APPS pots to high position (>50%)
   - Set TPS pots to low position (<40%)
   - ETB should go LOW immediately
   - Wait 1 second
   - IGN should go LOW
   - LED should fast blink
   - Return all pots to low position (<10%)
   - Wait 1 second
   - System should recover

### Interpreting Debug Output

**Voltage Readings (V):**
- Should match your calibration values at 0% and 100%
- Track sensor typically inverted from main sensor

**Percentage Readings (%):**
- Should be 0-100% range
- Main and track should agree within a few percent during normal operation

**Delta Values:**
- Delta APPS: Difference between APPS main and track (should be <10%)
- Delta TPS: Difference between TPS main and track (should be <10%)
- Delta APPS-TPS: Difference between APPS and TPS positions (should be <10%)

**Fault Status:**
- OK: No fault detected
- ACTIVE: Fault condition present

**Shutdown Status:**
- OK: Power enabled
- SHUTDOWN: Power cut to that system

**Recovery Timer:**
- IDLE: No recovery in progress
- RUNNING: Counting down 1 second before recovery

### LED Status Indicators

| LED Pattern | System State | Description |
|-------------|--------------|-------------|
| OFF | Normal Operation | No faults detected |
| Slow Blink (250ms) | Early Fault | Fault detected, timer running |
| Solid ON | ETB Shutdown | APPS or TPS fault active |
| Fast Blink (100ms) | Full Shutdown | Ignition cut, severe fault |

## State Machine

| State | Condition | ETB Pin | IGN Pin | LED |
|-------|-----------|---------|---------|-----|
| Normal | No faults | HIGH | HIGH | OFF |
| Early Detection | Fault timer running | HIGH | HIGH | Slow Blink |
| ETB Shutdown | Sensor fault >100ms | LOW | HIGH | Solid ON |
| Full Shutdown | APPS-TPS fault >1s | LOW | LOW | Fast Blink |
| Recovery | TPS ≤10% for 1s, no faults | HIGH | HIGH | OFF |

## Configuration

### Adjusting Fault Thresholds

Default thresholds are set per regulations. To modify:
```cpp
// In updateFaults() function, change these values:
if (appsDelta > 10.0)      // APPS threshold (default 10%)
if (tpsDelta > 10.0)       // TPS threshold (default 10%)
if (appsTpsDelta > 10.0)   // APPS-TPS threshold (default 10%)
if (avgTps <= 10.0)        // Recovery threshold (default 10%)
```

### Adjusting Timing Thresholds
```cpp
// APPS fault timing (default 100ms)
if (millis() - appsFaultStart >= 100)

// TPS fault timing (default 100ms)
if (millis() - tpsFaultStart >= 100)

// APPS-TPS IGN shutdown timing (default 1000ms)
if (elapsed >= 1000)

// Recovery timing (default 1000ms)
if (millis() - recoveryStart >= 1000)
```

## Troubleshooting

### Problem: False Faults During Normal Operation

**Symptoms:** ETB shuts down unexpectedly, faults trigger with no apparent cause

**Possible Causes:**
- Incorrect sensor calibration
- Electrical noise on analog inputs
- Loose sensor connections
- Ground loop issues

**Solutions:**
1. Verify voltage ranges with multimeter
2. Add 0.1µF ceramic capacitors between analog inputs and GND
3. Check all connections are secure
4. Verify proper grounding between Arduino and sensors
5. Use shielded twisted-pair cable for sensor wiring

### Problem: System Won't Recover After Fault

**Symptoms:** Outputs remain LOW even after fault condition clears

**Possible Causes:**
- TPS not returning below 10%
- Throttle mechanically stuck
- Background fault still active

**Solutions:**
1. Check debug output: verify avgTPS is actually <10%
2. Verify throttle return spring mechanism
3. Ensure no other fault conditions are present
4. Manually close throttle and wait 1 second

### Problem: Debug Output is Garbled

**Symptoms:** Serial monitor shows escape codes like `[2J[H` mixed with data

**Cause:** Using incompatible terminal (Arduino IDE Serial Monitor)

**Solution:** Use PlatformIO Serial Monitor or other ANSI-compatible terminal

### Problem: Erratic LED Behavior

**Symptoms:** LED blinks inconsistently or in unexpected patterns

**Possible Causes:**
- Multiple fault conditions competing
- Timing threshold near boundary
- Electrical noise

**Solutions:**
1. Monitor debug output to identify fault source
2. Increase timing margins if needed
3. Check power supply stability

## Safety Warnings

**THIS IS SAFETY-CRITICAL CODE**

Before deploying to a vehicle, you MUST:

1. Complete thorough bench testing with simulated sensors
2. Verify all fault detection thresholds trigger correctly
3. Validate recovery mechanisms work as expected
4. Perform controlled vehicle testing in safe environment
5. Have code reviewed by qualified personnel
6. Comply with all applicable motorsport regulations

**DO NOT:**
- Deploy without extensive testing
- Modify safety thresholds without understanding implications
- Use as single point of safety (implement redundant systems)
- Rely on this code alone for safety-critical applications

**DISCLAIMER:**
This code is provided as-is without warranty. The user assumes ALL responsibility for proper implementation, testing, and validation. Authors and contributors are NOT liable for any injury, damage, or loss resulting from use of this code.

## License

MIT License - See [LICENSE](LICENSE) file for details.

Copyright (c) 2025 s0Appy

## Contributing

Contributions are welcome! Please:

1. Fork the repository
2. Create a feature branch (`git checkout -b feature/improvement`)
3. Commit your changes (`git commit -am 'Add new feature'`)
4. Push to the branch (`git push origin feature/improvement`)
5. Open a Pull Request

## Support

For issues, questions, or suggestions:
- Open an issue on GitHub
- Check existing issues for solutions
- Review troubleshooting section above

## References

- FSAE Rules: https://www.fsaeonline.com/
- Formula Student Rules: https://www.formulastudent.de/
- Arduino Reference: https://www.arduino.cc/reference/

## Version History

### v1.0.0 (2025-12)
- Initial release
- APPS/TPS plausibility checks
- ETB and IGN shutdown logic
- Recovery mechanism
- ANSI terminal debug output
