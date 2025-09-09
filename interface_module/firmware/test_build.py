#!/usr/bin/env python3

"""
Test script to debug PlatformIO build hooks
Run this to see what's in the build directory after a build
"""

import os
from pathlib import Path

def main():
    project_dir = Path(__file__).parent
    build_dir = project_dir / ".pio" / "build" / "esp32doit-devkit-v1"
    
    print("🔍 Build Debug Information")
    print(f"📁 Project: {project_dir}")
    print(f"📁 Build: {build_dir}")
    print()
    
    if build_dir.exists():
        print("✅ Build directory exists")
        
        # List all files in build directory
        print("\n📋 Files in build directory:")
        for file in sorted(build_dir.iterdir()):
            if file.is_file():
                size = file.stat().st_size
                print(f"   {file.name} ({size:,} bytes)")
        
        # Check for specific files
        firmware_bin = build_dir / "firmware.bin"
        spiffs_bin = build_dir / "spiffs.bin"
        
        print("\n🎯 Target files:")
        if firmware_bin.exists():
            size = firmware_bin.stat().st_size
            print(f"   ✅ firmware.bin ({size:,} bytes)")
        else:
            print("   ❌ firmware.bin not found")
            
        if spiffs_bin.exists():
            size = spiffs_bin.stat().st_size
            print(f"   ✅ spiffs.bin ({size:,} bytes)")
        else:
            print("   ❌ spiffs.bin not found (run: pio run --target buildfs)")
    else:
        print("❌ Build directory doesn't exist")
        print("   Run: pio run -e esp32doit-devkit-v1")
    
    # Check OTA directory
    ota_dir = project_dir / "ota_builds"
    print(f"\n📦 OTA directory: {ota_dir}")
    
    if ota_dir.exists():
        files = list(ota_dir.glob("*.bin"))
        if files:
            print("✅ OTA files found:")
            for file in sorted(files):
                size = file.stat().st_size
                print(f"   {file.name} ({size:,} bytes)")
        else:
            print("⚠️  OTA directory exists but no .bin files")
    else:
        print("❌ OTA directory doesn't exist")

if __name__ == "__main__":
    main()