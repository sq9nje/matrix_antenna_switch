# 6x2 Matrix Antenna Switch

A complete antenna switching system that allows connecting two transceivers to up to 6 antennas with intelligent control and protection.

## Overview

This project implements a 6x2 matrix antenna switch system consisting of two main modules:

- **Relay Module**: High-power RF switching matrix with electromechanical relays
- **Interface Module**: ESP32-based control system with RS485 communication

The system provides flexible antenna routing while preventing simultaneous connection of both transceivers to the same antenna, ensuring equipment protection and operational safety.

## Key Features

### RF Performance
- **Power Handling**: Up to 1.5kW (full amateur license power)
- **Frequency Range**: DC to 50+ MHz with good performance
- **Protection**: Gas discharge tubes on all antenna ports for lightning/ESD protection
- **Isolation**: Decent port-to-port isolation
  - Better than 75dB below 10MHz
  - Better than 65dB below 30MHz
  - Better than 50dB below 50MHz
![Isolation Performance](/images/6x2_iso.png?raw=true "RF Isolation Performance")


### Control System
- **ESP32-based**: WiFi-enabled microcontroller with comprehensive firmware
- **Multiple Interfaces**: Web UI, REST API, WebSocket, and serial command control
- **Network Features**: mDNS discovery (`antenna.local`), OTA updates, WiFi management
- **Smart Features**: Antenna conflict prevention, single radio mode, persistent settings
- **RS485 Interface**: Reliable half-duplex communication with automatic direction detection
- **Expandability**: SAO (Simple Add-On) connector for future enhancements

### Design Features
- **Modular Architecture**: Separate relay and interface modules for flexibility
- **Three-Relay Configuration**: Prevents transceiver conflicts while maintaining isolation
- **Professional PCB Design**: Multi-layer boards with proper grounding and shielding
- **Easy Assembly**: Through-hole components for straightforward construction



## System Architecture

The complete system consists of:

1. **[Relay Module](relay_module/hardware/README.md)**:
   - 6 antenna sections, each with 3 relays
   - Allows connection to Radio 1, Radio 2, or 50Ω dummy load
   - Comprehensive protection circuitry

2. **[Interface Module](interface_module/hardware/README.md)**: Provides intelligent control and communication
   - ESP32 DevKit v1 for processing and connectivity
   - RS485 interface for remote control
   - Expansion connector for additional features

## Documentation

### Hardware Documentation
- **[Relay Module Hardware](relay_module/hardware/README.md)** - Complete RF switching matrix documentation
- **[Interface Module Hardware](interface_module/hardware/README.md)** - ESP32 control system hardware

### Firmware Documentation  
- **[Interface Module Firmware](interface_module/firmware/README.md)** - ESP32 firmware with web interface, REST API, and OTA updates
