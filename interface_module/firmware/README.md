# ESP32 6x2 Antenna Switch - Interface Module Firmware

Modern ESP32-based firmware for the 6x2 matrix antenna switch, providing comprehensive control through web interface, REST API, WebSocket, and serial commands with Over-The-Air (OTA) update capabilities.

## Overview

This firmware transforms the antenna switch into a smart, network-connected device that can be controlled remotely through multiple interfaces:

- **🌐 Web Interface**: Intuitive browser-based control panel
- **🔗 REST API**: HTTP endpoints for integration with external systems
- **⚡ WebSocket**: Real-time bidirectional communication
- **📟 Serial Commands**: Direct UART control for automation
- **🔄 OTA Updates**: Wireless firmware and filesystem updates
- **🏠 mDNS Discovery**: Automatic network discovery (`antenna.local`)

## Features

### Control Interfaces
- **Web UI**: Modern responsive interface at `http://antenna.local`
- **REST API**: 20+ endpoints for programmatic control
- **WebSocket**: Real-time state updates and control
- **Serial**: Simple text commands via USB or RS-485

### Smart Features
- **Antenna Swapping Prevention**: Automatic conflict resolution
- **Single Radio Mode**: Lock to one radio for simplex operation
- **Safety Interlocks**: Relay disconnect during firmware updates
- **Persistent Settings**: Configuration survives power cycles
- **Real-time Feedback**: Instant status updates across all interfaces

### Network & Updates
- **WiFi Manager**: Easy network configuration with fallback AP mode
- **mDNS**: Access via `antenna.local` hostname
- **OTA Updates**: Wireless firmware and web file updates
- **Dual Upload**: Maintains USB serial compatibility for development

## Hardware Requirements

- **ESP32 DevKit V1** (or compatible)
- **6x2 Antenna Switch Module** (relay control board)
- **5V Power Supply** (adequate for ESP32 + relays)

### Pin Assignments
```
Radio 1 Relays: GPIO 18, 5, 17, 16, 4, 2    (Antennas 1-6)
Radio 2 Relays: GPIO 15, 13, 12, 14, 27, 26 (Antennas 1-6)
Status LED:     GPIO 25
```

## Quick Start

### 1. Build and Flash
```bash
# Clone and setup
git clone <repository>
cd interface_module/firmware

# Build firmware
pio run -e esp32doit-devkit-v1

# Upload via USB
pio run -e esp32doit-devkit-v1 -t upload

# Upload web files
pio run -e esp32doit-devkit-v1 -t uploadfs
```

### 2. Initial Setup
1. **Connect to WiFi**: Device creates `AntennaSwitch-XXXX` AP for initial setup
2. **Configure Network**: Use WiFi Manager portal to connect to your network
3. **Access Interface**: Navigate to `http://antenna.local` or device IP

### 3. Basic Operation
- **Web Control**: Use browser interface for manual switching
- **API Integration**: Connect external software via REST API
- **Serial Control**: Send commands via USB serial at 115200 baud

## Documentation

### 📚 Complete API References
- **[REST & WebSocket API](REST_WebSocket_API.md)** - HTTP endpoints and real-time WebSocket communication
- **[Serial Commands](SERIAL_COMMANDS.md)** - UART command interface for automation and integration

### 🔧 Build & Deployment
- **[OTA Build Guide](OTA_BUILD_GUIDE.md)** - Firmware building, OTA updates, and deployment workflows

### 📋 Quick Reference Cards

#### Web Interface URLs
- **Main Control**: `http://antenna.local/`
- **Settings**: `http://antenna.local/settings`
- **System Status**: `http://antenna.local/status`
- **OTA Updates**: `http://antenna.local/ota`

#### Common API Endpoints
```bash
# Get current antenna state
curl http://antenna.local/api/state

# Switch Radio 1 to Antenna 3
curl -X POST http://antenna.local/api/antennas \\
  -H "Content-Type: application/json" \\
  -d '{"1":3}'

# Get system status
curl http://antenna.local/api/status
```

#### Serial Commands
```bash
set 1 3      # Switch Radio 1 to Antenna 3
get 1        # Get current antenna for Radio 1
?            # Device information
test         # Test all relays
```

## Architecture

### Core Components
- **`main.cpp`**: Application entry point and main loop
- **`antenna_hardware.cpp`**: Relay control and switching logic
- **`web_server.cpp`**: HTTP server and REST API endpoints
- **`websocket.cpp`**: Real-time WebSocket communication
- **`command_parser.cpp`**: Serial command processing
- **`wifi_manager.cpp`**: Network configuration and management

### Web Assets (`data/` directory)
- **`index.html`**: Main control interface
- **`settings.html`**: Configuration page
- **`status.html`**: System information display
- **`ota.html`**: Firmware update interface
- **`style.css`**: Responsive UI styling
- **`script.js`**: Client-side JavaScript logic

## Development

### Build Environments
```ini
# USB Development (default)
[env:esp32doit-devkit-v1]
upload_port = /dev/ttyUSB0

# Wireless OTA Updates
[env:esp32doit-devkit-v1-ota]
upload_protocol = espota
upload_port = antenna.local
```

### Dependencies
- **ESPAsyncWebServer**: HTTP server with async support
- **WebSocketsServer**: Real-time bidirectional communication
- **ArduinoJson**: JSON parsing and generation
- **WiFiManager**: Network configuration portal
- **ArduinoOTA**: Over-the-air update support

### Build Scripts
- **`build_ota.sh`**: Complete build with automatic file copying
- **`copy_ota_files.py`**: Manual file copying for OTA deployment
- **`build_version.py`**: Automatic versioning and timestamp injection

## Configuration

### WiFi Setup
1. **Initial Setup**: Device creates `AntennaSwitch-XXXX` access point
2. **Connect**: Join AP and navigate to `192.168.4.1`
3. **Configure**: Select your WiFi network and enter credentials
4. **Automatic**: Device connects to your network and starts mDNS

### Default Settings
- **Hostname**: `antenna` (accessible via `antenna.local`)
- **OTA Password**: `antenna123`
- **WebSocket Port**: `81`
- **Serial Baud**: `115200`

### Customization
- **Antenna Names**: Configure via web interface or API
- **Operation Modes**: Single radio mode, antenna swapping prevention
- **Network Settings**: Static IP, custom hostname via web settings

## Troubleshooting

### Common Issues
**Device Not Accessible:**
- Check WiFi connection and signal strength
- Try IP address instead of `antenna.local`
- Reset network settings via serial: `AT+RST`

**OTA Update Fails:**
- Verify file type (firmware vs SPIFFS)
- Check available flash space
- Ensure stable WiFi connection during upload

**Relays Not Switching:**
- Check power supply capacity (relays need adequate current)
- Verify pin connections against hardware documentation
- Use `test` serial command to verify hardware operation

### Debug Access
- **Serial Monitor**: Connect USB, open serial terminal at 115200 baud
- **System Status**: Check `/status` page for detailed system information
- **API Status**: Use `/api/status` endpoint for JSON system data
