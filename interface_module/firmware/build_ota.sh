#!/bin/bash

# OTA Build Script for ESP32 Antenna Switch
# This script builds both firmware and filesystem images for OTA updates

echo "=== ESP32 Antenna Switch OTA Build Script ==="
echo ""

# Check if PlatformIO is available
if ! command -v pio &> /dev/null; then
    echo "❌ PlatformIO CLI not found. Please install PlatformIO first."
    exit 1
fi

# Create OTA builds directory
mkdir -p ota_builds
rm -f ota_builds/*

echo "🔨 Building firmware..."
pio run -e esp32doit-devkit-v1
if [ $? -ne 0 ]; then
    echo "❌ Firmware build failed!"
    exit 1
fi

echo "🔨 Building filesystem..."
pio run -e esp32doit-devkit-v1 --target buildfs
if [ $? -ne 0 ]; then
    echo "❌ Filesystem build failed!"
    exit 1
fi

echo ""
echo "✅ Build completed successfully!"
echo ""
echo "📝 Note: OTA files are automatically copied to ota_builds/ directory by the build system"
echo "         Check the build output above for file locations and latest links"
echo ""
echo "📤 Upload instructions:"
echo "   1. Web interface: Upload .bin files via http://antenna.local/ota"
echo "   2. PlatformIO USB upload: pio run -e esp32doit-devkit-v1 -t upload"
echo "   3. PlatformIO OTA upload: pio run -e esp32doit-devkit-v1-ota -t upload"
echo ""

# Show file sizes if directory exists
if [ -d "ota_builds" ]; then
    echo "📊 Available OTA files:"
    ls -lh ota_builds/*.bin 2>/dev/null | awk '{print "   " $9 ": " $5}' || echo "   No .bin files found"
else
    echo "📁 OTA builds directory will be created automatically during build"
fi