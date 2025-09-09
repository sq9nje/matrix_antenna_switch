# ESP32 Antenna Switch REST & WebSocket API

This document describes the REST and WebSocket API endpoints for the 6x2 ESP32 antenna switch interface module.

## Table of Contents

- [ESP32 Antenna Switch REST \& WebSocket API](#esp32-antenna-switch-rest--websocket-api)
  - [Table of Contents](#table-of-contents)
  - [Base URL \& Authentication](#base-url--authentication)
  - [Content Types](#content-types)
  - [Antenna Management](#antenna-management)
    - [Get All Antenna Names](#get-all-antenna-names)
    - [Update All Antenna Names](#update-all-antenna-names)
    - [Get Single Antenna Info](#get-single-antenna-info)
    - [Update Single Antenna Name](#update-single-antenna-name)
  - [Switch State](#switch-state)
    - [Get Current State](#get-current-state)
  - [Configuration](#configuration)
    - [Get Hostname](#get-hostname)
    - [Update Hostname](#update-hostname)
    - [Get Operation Mode](#get-operation-mode)
    - [Update Operation Mode](#update-operation-mode)
  - [Device Status](#device-status)
    - [Get System Status](#get-system-status)
  - [System Administration](#system-administration)
    - [Reboot Device](#reboot-device)
    - [Reset Network Settings](#reset-network-settings)
  - [OTA Updates](#ota-updates)
    - [Upload Firmware/SPIFFS](#upload-firmwarespiffs)
  - [WebSocket API](#websocket-api)
    - [Connection](#connection)
    - [Client → Server Messages](#client--server-messages)
      - [Switch Antenna](#switch-antenna)
    - [Server → Client Messages](#server--client-messages)
      - [Current State Update](#current-state-update)
      - [Antenna Names Update](#antenna-names-update)
      - [OTA Progress Updates](#ota-progress-updates)
    - [Connection Events](#connection-events)
      - [New Client Connection](#new-client-connection)
      - [Client Disconnection](#client-disconnection)
  - [Error Codes](#error-codes)
  - [Examples](#examples)
    - [Switch Radio 1 to Antenna 3 via WebSocket](#switch-radio-1-to-antenna-3-via-websocket)
    - [Monitor Real-time State Changes](#monitor-real-time-state-changes)
    - [Get Device IP and Connection Info](#get-device-ip-and-connection-info)
    - [Update Antenna Names](#update-antenna-names)

---

## Base URL & Authentication

**Base URL:**
- Default: `http://antenna.local` (via mDNS)
- Alternative: `http://<device_ip>`
- WebSocket: `ws://<host>:81/`

**Authentication:**
No authentication required for current implementation.

## Content Types
- **Request**: `application/json` for POST/PUT bodies
- **Response**: `application/json` for data endpoints, `text/plain` for status messages

---

## Antenna Management

### Get All Antenna Names
```http
GET /api/antennas
```
**Response:**
```json
["Dipole", "Yagi", "Loop", "Vertical", "Antenna 5", "Antenna 6"]
```

### Update All Antenna Names
```http
POST /api/antennas
Content-Type: application/json

{
  "0": "New Name 1",
  "1": "New Name 2",
  "2": "New Name 3"
}
```
**Response:** `200 OK`

### Get Single Antenna Info
```http
GET /api/antenna/{index}
```
**Parameters:**
- `index`: Antenna index (0-5)

**Response:**
```json
{
  "index": 0,
  "name": "Dipole"
}
```

### Update Single Antenna Name
```http
PUT /api/antenna/{index}
Content-Type: application/json

{
  "name": "New Antenna Name"
}
```
**Response:** `200 OK`

---

## Switch State

### Get Current State
```http
GET /api/state
```
**Response:**
```json
{
  "radio1": 1,
  "radio2": 3
}
```
**Note:** Values 0 = disconnected, 1-6 = antenna index

---

## Configuration

### Get Hostname
```http
GET /api/hostname
```
**Response:**
```json
{
  "hostname": "antenna"
}
```

### Update Hostname
```http
POST /api/hostname
Content-Type: application/json

{
  "hostname": "my-antenna-switch"
}
```
**Response:** `200 OK - Restart required for changes to take effect`

### Get Operation Mode
```http
GET /api/operation-mode
```
**Response:**
```json
{
  "antennaSwapping": true,
  "singleRadioMode": false
}
```

### Update Operation Mode
```http
POST /api/operation-mode
Content-Type: application/json

{
  "antennaSwapping": false,
  "singleRadioMode": true
}
```
**Response:** `200 OK`

---

## Device Status

### Get System Status
```http
GET /api/status
```
**Response:**
```json
{
  "ssid": "MyWiFi",
  "ip": "192.168.1.100",
  "gateway": "192.168.1.1",
  "subnet": "255.255.255.0",
  "dns": "192.168.1.1",
  "rssi": -45,
  "macAddress": "AA:BB:CC:DD:EE:FF",
  "hostname": "antenna",
  "firmwareVersion": "1.0.0",
  "buildTime": "20240909_143022",
  "chipModel": "ESP32-D0WD-V3",
  "chipRevision": 3,
  "cpuFreqMHz": 240,
  "freeHeap": 123456,
  "totalHeap": 327680,
  "uptime": 3600,
  "currentRadio1": 1,
  "currentRadio2": 0
}
```

---

## System Administration

### Reboot Device
```http
POST /api/reboot
```
**Response:** `200 Device rebooting...`

### Reset Network Settings
```http
POST /api/reset-network
```
**Response:** `200 Network settings reset. Device will reboot...`

---

## OTA Updates

### Upload Firmware/SPIFFS
```http
POST /api/update
Content-Type: multipart/form-data

form-data: firmware=<binary_file>
```
**File Type Detection:**
- Files containing "firmware" → Firmware update
- Files containing "spiffs" → SPIFFS/filesystem update
- Generic `.bin` files → Assumed firmware

**Response:** `200 Update Success` or `200 Update Failed`

**Progress Updates:** Real-time progress sent via WebSocket:
```json
{
  "type": "ota",
  "status": "progress",
  "progress": 75,
  "message": ""
}
```

---

## WebSocket API

### Connection
Connect to `ws://<host>:81/` for real-time bidirectional communication.

### Client → Server Messages

#### Switch Antenna
```json
{
  "type": "select",
  "radio": 1,
  "antenna": 3
}
```
**Parameters:**
- `radio`: Radio number (1 or 2)
- `antenna`: Antenna number (1-6, or 0 to disconnect)

### Server → Client Messages

#### Current State Update
Sent automatically when antenna state changes or client connects:
```json
{
  "type": "state",
  "radio1": 2,
  "radio2": 0,
  "singleRadioMode": false
}
```
**Fields:**
- `radio1`/`radio2`: Current antenna (0 = disconnected, 1-6 = antenna number)
- `singleRadioMode`: Whether single radio mode is enabled

#### Antenna Names Update
Sent when antenna names are changed or client connects:
```json
{
  "type": "antennaNames",
  "names": ["20m Dipole", "40m Yagi", "80m Loop", "Vertical", "Antenna 5", "Antenna 6"]
}
```

#### OTA Progress Updates
Real-time firmware/SPIFFS upload progress:
```json
{
  "type": "ota",
  "status": "starting|progress|complete|error",
  "progress": 75,
  "message": "Status message"
}
```
**Status Values:**
- `starting`: Upload beginning
- `progress`: Upload in progress (see `progress` field 0-100)
- `complete`: Upload successful, device restarting
- `error`: Upload failed (see `message` for details)

### Connection Events

#### New Client Connection
When a client connects:
1. Server automatically sends current `state` message
2. Server automatically sends current `antennaNames` message

#### Client Disconnection
Server logs disconnection but takes no other action.

---

## Error Codes

- `200`: Success
- `400`: Bad Request (invalid parameters/JSON)
- `404`: Not Found (invalid endpoint/antenna index)
- `500`: Internal Server Error

## Examples

### Switch Radio 1 to Antenna 3 via WebSocket
```javascript
const ws = new WebSocket('ws://antenna.local:81/');
ws.onopen = function() {
  ws.send(JSON.stringify({
    type: "select",
    radio: 1,
    antenna: 3
  }));
};
```

### Monitor Real-time State Changes
```javascript
const ws = new WebSocket('ws://antenna.local:81/');
ws.onmessage = function(event) {
  const data = JSON.parse(event.data);
  if (data.type === 'state') {
    console.log(`Radio 1: ${data.radio1}, Radio 2: ${data.radio2}`);
  } else if (data.type === 'antennaNames') {
    console.log('Antenna names updated:', data.names);
  }
};
```

### Get Device IP and Connection Info
```bash
curl http://antenna.local/api/status | jq '.ip, .ssid, .rssi'
```

### Update Antenna Names
```bash
curl -X POST http://antenna.local/api/antennas \
  -H "Content-Type: application/json" \
  -d '{"0":"20m Dipole","1":"40m Yagi","2":"80m Loop"}'
```
