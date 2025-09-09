# ESP32 Antenna Switch Serial Commands

This document describes the serial command interface for the 6x2 ESP32 antenna switch interface module.

## Table of Contents

- [Connection Setup](#connection-setup)
- [Command Format](#command-format)
- [Available Commands](#available-commands)
  - [Switch Antenna](#switch-antenna-set)
  - [Get Current Antenna](#get-current-antenna-get)
  - [Device Information](#device-information-)
  - [LED Blink Test](#led-blink-test-blink)
  - [Full System Test](#full-system-test-test)
- [Response Codes](#response-codes)
- [Examples](#examples)
- [Troubleshooting](#troubleshooting)

---

## Connection Setup

### Serial Ports
The device supports command input on two serial interfaces:

- **UART0 (USB Serial)**: Primary interface for debugging and development
  - Baud rate: 115200 (default)
  - Data bits: 8, Parity: None, Stop bits: 1
  - Connect via USB cable to computer

- **UART2 (RS-485/External)**: Secondary interface for remote control
  - Same serial parameters as UART0
  - Responses are sent to UART0 (USB) for debugging

### Connection Examples
```bash
# Linux/macOS - USB serial
screen /dev/ttyUSB0 115200
# or
minicom -D /dev/ttyUSB0 -b 115200

# Windows - USB serial  
PuTTY - Serial, COM3, 115200

# Arduino IDE Serial Monitor
Tools → Serial Monitor, 115200 baud
```

---

## Command Format

### Structure
```
<command> [parameters]<CR><LF>
```

### Rules
- Commands are **case-insensitive** (automatically converted to lowercase)
- Parameters are separated by **single spaces**
- Commands must end with **carriage return (CR)** and/or **line feed (LF)**
- Maximum command length: **31 characters** (including parameters)
- Invalid or incomplete commands are ignored silently

---

## Available Commands

### Switch Antenna: `set`
Switch a radio to a specific antenna.

**Syntax:**
```
set <radio> <antenna>
```

**Parameters:**
- `radio`: Radio number (1 or 2)
- `antenna`: Antenna number (1-6, or 0 to disconnect)

**Responses:**
- `+OK`: Antenna switched successfully
- `!ERR`: Invalid parameters (radio/antenna out of range)
- `!BUSY`: Single radio mode enabled and trying to switch radio 2

**Examples:**
```bash
set 1 3      # Switch Radio 1 to Antenna 3
set 2 0      # Disconnect Radio 2
set 1 6      # Switch Radio 1 to Antenna 6
set 2 1      # Switch Radio 2 to Antenna 1
```

### Get Current Antenna: `get`
Query the current antenna for a specific radio.

**Syntax:**
```
get <radio>
```

**Parameters:**
- `radio`: Radio number (1 or 2)

**Response:**
- `0`: Radio disconnected
- `1-6`: Antenna number currently connected

**Examples:**
```bash
get 1        # Returns: 3 (if Radio 1 is on Antenna 3)
get 2        # Returns: 0 (if Radio 2 is disconnected)
```

### Device Information: `?`
Get device identification information.

**Syntax:**
```
?
```

**Response:**
```
6x2 Antenna Switch SQ9NJE
```

### LED Blink Test: `blink`
Blink the status LED for testing/identification purposes.

**Syntax:**
```
blink <count>
```

**Parameters:**
- `count`: Number of blinks (1-255)

**Examples:**
```bash
blink 3      # Blink LED 3 times
blink 10     # Blink LED 10 times
```

### Full System Test: `test`
Perform a complete relay test sequence.

**Syntax:**
```
test
```

**Behavior:**
1. Cycles through all antennas on Radio 1 (6→5→4→3→2→1→0)
2. Cycles through all antennas on Radio 2 (6→5→4→3→2→1→0)
3. Each step has 100ms delay
4. Total test time: ~1.4 seconds

**Use Cases:**
- Verify all relays are working
- Audio/visual confirmation of relay operation
- Factory testing and troubleshooting

---

## Response Codes

| Code | Meaning | Description |
|------|---------|-------------|
| `+OK` | Success | Command executed successfully |
| `!ERR` | Error | Invalid parameters or command failed |
| `!BUSY` | Busy/Blocked | Operation blocked (e.g., single radio mode) |
| `0-6` | Status | Current antenna number (0=disconnected) |

---

## Examples

### Basic Antenna Switching
```bash
# Check current state
get 1          # Returns: 0 (disconnected)
get 2          # Returns: 0 (disconnected)

# Switch antennas
set 1 2        # Response: +OK
get 1          # Returns: 2

set 2 5        # Response: +OK  
get 2          # Returns: 5

# Disconnect radio
set 1 0        # Response: +OK
get 1          # Returns: 0
```

### Device Testing Sequence
```bash
# Identify device
?              # Returns: 6x2 Antenna Switch SQ9NJE

# Test LED
blink 5        # LED blinks 5 times

# Test all relays
test           # All relays cycle through sequence

# Manual relay verification  
set 1 1        # Switch to antenna 1
set 1 2        # Switch to antenna 2
set 1 3        # Switch to antenna 3
# ... continue testing each antenna
```

### Error Handling
```bash
# Invalid radio number
set 3 1        # Response: !ERR

# Invalid antenna number  
set 1 7        # Response: !ERR

# Single radio mode blocking
set 2 1        # Response: !BUSY (if single radio mode enabled)
```

### Arduino/ESP32 Integration
```cpp
// Send command via Serial
Serial.println("set 1 3");
delay(100);

// Read response
if (Serial.available()) {
  String response = Serial.readString();
  if (response.indexOf("+OK") >= 0) {
    // Success
  } else if (response.indexOf("!ERR") >= 0) {
    // Error
  }
}
```

---

## Troubleshooting

### Common Issues

**No Response to Commands:**
- Check baud rate (should be 115200)
- Verify line endings (CR/LF required)
- Ensure command length < 32 characters
- Try device info command: `?`

**Commands Ignored:**
- Check for proper command termination (Enter key)
- Verify parameters are within valid ranges
- Commands are processed only when complete line received

**Relay Not Switching:**
- Use `test` command to verify hardware
- Check power supply (relays need adequate current)
- Verify connections with multimeter
- Try `blink` command to confirm device is responding

**Error Responses:**
- `!ERR`: Check parameter ranges (radio: 1-2, antenna: 0-6)
- `!BUSY`: Check if single radio mode is enabled via web interface

### Debug Information
All commands sent to UART2 (RS-485) will show responses on UART0 (USB) for debugging purposes.

### Serial Monitor Settings
- **Baud Rate**: 115200
- **Line Ending**: Both NL & CR (or just CR)
- **Local Echo**: Enabled (helpful for command verification)

---

## Integration Notes

### RS-485 Communication
- UART2 is intended for RS-485 remote control
- Use proper RS-485 transceiver circuit
- Responses are echoed to USB serial for debugging
- Implement proper bus arbitration for multi-drop networks

### Automation Scripts
The simple command structure makes it easy to integrate with:
- Shell scripts (`echo "set 1 3" > /dev/ttyUSB0`)
- Python serial libraries
- Ham radio logging software
- Contest automation systems
- Microcontroller projects

### Command Buffering
- Input buffer size: 32 characters
- Commands are processed line-by-line
- Partial commands are held until CR/LF received
- Buffer overflow protection (excess characters ignored)