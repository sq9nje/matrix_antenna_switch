# OTA Build Guide for ESP32 Antenna Switch

This guide explains how to build firmware and filesystem images for Over-The-Air (OTA) updates.

## Quick Start

### Option 1: Manual Build + Copy (Recommended)
```bash
# Build firmware
pio run -e esp32doit-devkit-v1

# Build filesystem (optional)
pio run -e esp32doit-devkit-v1 --target buildfs

# Copy files for OTA upload
python3 copy_ota_files.py
```

### Option 2: Use the Build Script
```bash
./build_ota.sh
```

### Option 3: Individual Commands
```bash
# Build firmware only
pio run -e esp32doit-devkit-v1

# Build filesystem only  
pio run -e esp32doit-devkit-v1 --target buildfs

# Copy to OTA directory when ready
python3 copy_ota_files.py
```

## Output Files

After building, you'll find these files in the `.pio/build/esp32doit-devkit-v1/` directory:

- **`firmware.bin`** - Main application firmware
- **`spiffs.bin`** - Web interface and configuration files

## Using OTA Files

### Web Interface Method
1. Connect to your antenna switch web interface
2. Go to Settings → "Firmware Update (OTA)"
3. Upload the appropriate `.bin` file:
   - Use `firmware.bin` for firmware updates
   - Use `spiffs.bin` to update web files

### PlatformIO Upload Methods
```bash
# Upload firmware over USB serial (default)
pio run -e esp32doit-devkit-v1 -t upload

# Upload filesystem over USB serial
pio run -e esp32doit-devkit-v1 -t uploadfs

# Upload firmware over WiFi (OTA)
pio run -e esp32doit-devkit-v1-ota -t upload

# Upload filesystem over WiFi (OTA)
pio run -e esp32doit-devkit-v1-ota -t uploadfs
```

## Build Script Features

The `build_ota.sh` script provides:

- ✅ Automatic build of both firmware and filesystem
- 📦 Timestamped filenames for version tracking
- 🔗 Latest symlinks for easy access
- 📊 File size information
- ❌ Error checking and validation

## File Management

Due to PlatformIO post-build hook limitations, files are **not** automatically copied. Instead, use the manual copy approach:

### Standard Workflow
1. **Build your project** using any method (CLI, VSCode, etc.)
2. **Copy files** using: `python3 copy_ota_files.py`
3. **Upload via web interface** at `http://antenna.local/ota`

### Copy Script Features

The `copy_ota_files.py` script provides:

- 📁 Creates `ota_builds/` directory automatically
- 📦 Copies both firmware and SPIFFS files with timestamps
- 🏷️ Creates timestamped filenames: `antenna_switch_firmware_20240909_143022.bin`
- 🔗 Maintains `firmware_latest.bin` and `spiffs_latest.bin` links
- 📊 Shows file sizes and upload instructions
- ✅ Works with any build method (CLI, VSCode, Arduino IDE)

### Build + Copy Examples

```bash
# Standard workflow
pio run -e esp32doit-devkit-v1          # Build firmware
python3 copy_ota_files.py               # Copy for OTA

# With filesystem
pio run -e esp32doit-devkit-v1 --target buildfs  # Build SPIFFS
python3 copy_ota_files.py                        # Copy both files

# Using build script (does both)
./build_ota.sh
```

## Build Outputs

Files are created in the `ota_builds/` directory:

```
ota_builds/
├── antenna_switch_firmware_20240909_143022.bin
├── antenna_switch_spiffs_20240909_143022.bin
├── firmware_latest.bin -> antenna_switch_firmware_20240909_143022.bin
└── spiffs_latest.bin -> antenna_switch_spiffs_20240909_143022.bin
```

## Safety Notes

⚠️ **Important**: During OTA updates:
- All antenna relays are automatically disconnected
- Don't power off the device during updates
- Web interface shows real-time progress
- Device automatically restarts after successful update

## Troubleshooting

### Build Fails
- Check PlatformIO installation: `pio --version`
- Clean build: `pio run --target clean`
- Update dependencies: `pio pkg update`

### OTA Upload Fails
- Verify device is connected to WiFi
- Check mDNS hostname (default: `antenna.local`)
- Ensure OTA password is correct (`antenna123`)
- Try using IP address instead of hostname

### File Too Large
- Check available flash space: ESP32 has ~1.2MB for firmware
- SPIFFS partition size is defined in partition scheme
- Consider optimizing build flags for size

## Advanced Configuration

### Partition Scheme
The default partition scheme allocates:
- ~1.2MB for firmware
- ~1.5MB for SPIFFS
- Remaining for system use

### OTA Password
Default password is `antenna123`. To change:
1. Edit `main.cpp` line containing `setPassword`
2. Update `platformio.ini` upload flags
3. Rebuild and upload

## Version Information

The build system automatically adds:
- Build timestamp (format: YYYYMMDD_HHMMSS)
- Firmware version (currently 1.0.0)
- These are displayed on the device status page at `/status`

### Viewing Version Information
- **Web Interface**: Go to `/status` to see firmware version and build time
- **Serial Output**: Version info is displayed during boot
- **API**: Available via `/api/status` endpoint

## Integration with CI/CD

The build script can be integrated into automated workflows:

```bash
#!/bin/bash
# CI/CD example
cd firmware/
./build_ota.sh
if [ $? -eq 0 ]; then
    echo "Build successful, uploading artifacts..."
    # Upload to artifact storage
fi
```