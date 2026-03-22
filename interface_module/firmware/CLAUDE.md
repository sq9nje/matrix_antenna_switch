# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

ESP32-based firmware for a 6x2 matrix antenna switch that provides web, REST API, WebSocket, and serial control interfaces. The system allows two radios to connect to six antennas with automatic conflict resolution and OTA update capabilities.

## Build Commands

### Standard Build & Upload
```bash
# Build firmware for USB upload
pio run -e esp32doit-devkit-v1

# Upload firmware via USB
pio run -e esp32doit-devkit-v1 -t upload

# Build filesystem (web interface files)
pio run -e esp32doit-devkit-v1 --target buildfs

# Upload filesystem via USB
pio run -e esp32doit-devkit-v1 -t uploadfs
```

### OTA Build Workflow
```bash
# Option 1: Manual build + copy (recommended)
pio run -e esp32doit-devkit-v1
pio run -e esp32doit-devkit-v1 --target buildfs
python3 copy_ota_files.py

# Option 2: Use build script (does everything)
./build_ota.sh

# Upload firmware over WiFi (OTA)
pio run -e esp32doit-devkit-v1-ota -t upload

# Upload filesystem over WiFi (OTA)
pio run -e esp32doit-devkit-v1-ota -t uploadfs
```

### Testing & Debugging
```bash
# Open serial monitor
pio device monitor -b 115200

# Clean build
pio run --target clean

# Update dependencies
pio pkg update
```

## Architecture

### Core State Management
The firmware maintains global state in `globals.cpp`:
- `currentAntenna[2]`: Current antenna selection for each radio (0=disconnected, 1-6=antenna number)
- `antennas[6]`: Array of `AntennaConfig` structs, each with `name` (String) and `bands` (std::vector\<String\>), empty vector means no bands selected
- `antennaSwappingEnabled`: Allows automatic swapping when both radios select same antenna
- `singleRadioMode`: Locks system to radio 1 only

### Module Responsibilities

**`main.cpp`**: Application entry point
- Initializes all subsystems in sequence (storage → hardware → network → OTA → web server)
- Main loop handles OTA, WebSocket, and dual serial port command processing
- UART0 (USB Serial) and UART2 (RS-485) both process commands, with responses echoed to USB for debugging

**`antenna_hardware.cpp`**: Relay control logic
- `selectAntenna()` is the critical function that handles all antenna switching logic
- Implements antenna swapping: if both radios want the same antenna and swapping is enabled, they exchange antennas
- Single radio mode automatically disconnects radio 2
- All relay operations trigger WebSocket state broadcasts to connected clients

**`web_server.cpp`**: REST API implementation
- 20+ endpoints for antenna management, configuration, and system status
- Handles web file serving from SPIFFS
- OTA update endpoint with real-time progress via WebSocket
- During OTA updates, all relays are automatically disconnected for safety

**`websocket.cpp`**: Real-time bidirectional communication
- Runs on port 81 (separate from HTTP server on port 80)
- Broadcasts state changes to all connected clients automatically
- New clients receive current state and antenna names on connection
- Used for OTA progress updates during firmware uploads

**`command_parser.cpp`**: Serial command processor
- Case-insensitive command parsing (commands auto-converted to lowercase)
- 32-character buffer for command input
- Processes commands from both UART0 and UART2 simultaneously

**`storage.cpp`**: SPIFFS-based persistence
- All settings stored in a single `/settings.json` file (hostname, operation modes, antenna names)
- Uses JSON format for serialization via ArduinoJson
- Settings survive power cycles and firmware updates

**`wifi_manager.cpp`**: Network configuration
- WiFiManager provides captive portal for initial setup
- Creates AP `AntennaSwitch-XXXX` if no configured network
- mDNS hostname: `antenna.local` (configurable via settings)

### Pin Configuration
Defined in `globals.h`:
```
Radio 1 Relays: GPIO 13, 12, 14, 27, 26, 25 (Antennas 1-6)
Radio 2 Relays: GPIO 5, 18, 19, 21, 22, 23  (Antennas 1-6)
Status LED:     GPIO 33
UART2 (RS-485): RX=16, TX=17
```

### Web Interface Files (`data/` directory)
- `index.html`: Main antenna control interface
- `settings.html`: System configuration (hostname, operation modes)
- `status.html`: System information and diagnostics
- `ota.html`: Firmware and filesystem update interface
- `script.js`: Shared JavaScript logic, WebSocket client
- `style.css`: Responsive UI styling

All web files are stored in SPIFFS and served by the async web server.

## Key Implementation Details

### Antenna Swapping Logic
When `selectAntenna()` is called:
1. Validates parameters (radio 0-1, antenna 0-6)
2. If single radio mode is active and radio 2 is selected, immediately disconnect radio 2
3. If antenna is already selected by other radio:
   - If swapping enabled: disconnect other radio → connect requesting radio → connect other radio to previous antenna
   - If swapping disabled: return error code 2 (busy)
4. Otherwise: disconnect current antenna → connect new antenna
5. Broadcast state change via WebSocket
6. Blink status LED once for success, three times for error

### OTA Safety Mechanism
When OTA update starts (in `main.cpp` `initializeOTA()`):
- All 12 relay pins are immediately set LOW
- Both `currentAntenna[]` values set to 0
- WebSocket notification sent to all clients
- Prevents RF transmission during firmware flash
- Device automatically reboots after successful update

### Build Versioning
`build_version.py` pre-build script automatically generates `build_version.h` with:
- Firmware version (1.0.0)
- Build timestamp (YYYYMMDD_HHMMSS format)
- Values accessible via `/api/status` and web status page

### Serial Command Processing
Commands from both UART0 (USB) and UART2 (RS-485) are processed identically:
- UART0 responses go to UART0
- UART2 responses go to UART0 (for debugging)
- Command buffer size: 32 characters
- Auto-converts to lowercase before parsing

## Configuration Files

**`/settings.json`** (SPIFFS, created at runtime):
- `mdnsHostname` (string): mDNS hostname (default: `antenna`)
- `antennaSwapping` (bool): Enable automatic antenna swapping between radios
- `singleRadioMode` (bool): Lock system to radio 1 only
- `antennas` (array of objects): Each with `name` (string) and `bands` (string array), e.g. `[{"name": "Dipole", "bands": ["20m","40m"]}, ...]`
- Exportable/importable via `/api/settings/export` and `/api/settings/import`

**`platformio.ini`**:
- Two environments: `esp32doit-devkit-v1` (USB) and `esp32doit-devkit-v1-ota` (WiFi)
- OTA password: `antenna123` (defined in both `platformio.ini` and `main.cpp`)
- Pre-build script: `build_version.py` for automatic versioning

## Development Workflow

### Making Code Changes
1. Edit source files in `src/` or headers in `include/`
2. Build with `pio run -e esp32doit-devkit-v1`
3. Upload via USB for initial flash or debugging
4. Use OTA for subsequent updates if device is on network

### Modifying Web Interface
1. Edit files in `data/` directory
2. Build filesystem: `pio run -e esp32doit-devkit-v1 --target buildfs`
3. Upload: `pio run -e esp32doit-devkit-v1 -t uploadfs` (USB) or `-ota` variant
4. Alternatively, build OTA files and upload via web interface

### Testing Changes
1. Serial monitor at 115200 baud: `pio device monitor -b 115200`
2. Use `test` command to verify relay operation
3. Check `/api/status` endpoint for system diagnostics
4. Monitor WebSocket messages in browser console

### Adding New API Endpoints
1. Add handler function in `web_server.cpp`
2. Register route in `initializeWebServer()` function
3. Follow existing patterns for JSON responses
4. Consider whether WebSocket broadcast is needed for state changes

### Modifying Antenna Logic
All relay control goes through `selectAntenna()` in `antenna_hardware.cpp`:
- Returns 0 for success, 1 for parameter error, 2 for busy
- Always call `sendWebSocketUpdate()` after state changes
- Blink LED for user feedback
- Validate all parameters before hardware changes

## Important Constants

- Default hostname: `antenna` (accessible as `antenna.local`)
- OTA password: `antenna123`
- WebSocket port: 81
- HTTP port: 80 (default)
- Serial baud rates: UART0=115200, UART2=9600
- Command buffer size: 32 characters
- Number of radios: 2
- Number of antennas: 6

## API Documentation
See `REST_WebSocket_API.md` for complete REST and WebSocket API reference.
See `SERIAL_COMMANDS.md` for serial command protocol details.
