#!/usr/bin/env python3

"""
Standalone script to copy firmware and SPIFFS files to OTA builds directory.
Use this if the automatic post-build actions don't work with VSCode PlatformIO extension.

Usage:
    python copy_ota_files.py
    
Or make it executable and run directly:
    chmod +x copy_ota_files.py
    ./copy_ota_files.py
"""

import os
import shutil
import datetime
from pathlib import Path

def main():
    # Get project directory (where this script is located)
    script_dir = Path(__file__).parent
    project_dir = script_dir
    
    # Standard PlatformIO build directory
    build_dir = project_dir / ".pio" / "build" / "esp32doit-devkit-v1"
    ota_dir = project_dir / "ota_builds"
    
    # Create OTA directory
    ota_dir.mkdir(exist_ok=True)
    
    # Generate timestamp
    build_time = datetime.datetime.now().strftime("%Y%m%d_%H%M%S")
    
    copied_files = []
    
    print("🔧 Manual OTA File Copy")
    print(f"📁 Project: {project_dir}")
    print(f"📁 Build dir: {build_dir}")
    print(f"📁 OTA dir: {ota_dir}")
    print()
    
    # Copy firmware if it exists
    firmware_src = build_dir / "firmware.bin"
    if firmware_src.exists():
        firmware_dst = ota_dir / f"antenna_switch_firmware_{build_time}.bin"
        shutil.copy2(str(firmware_src), str(firmware_dst))
        
        # Create/update latest link
        latest_link = ota_dir / "firmware_latest.bin"
        if latest_link.exists():
            latest_link.unlink()
        shutil.copy2(str(firmware_src), str(latest_link))
        
        copied_files.append(f"Firmware: {firmware_dst.name}")
        print(f"✅ Firmware copied: {firmware_dst.name}")
    else:
        print(f"⚠️  Firmware not found: {firmware_src}")
    
    # Copy SPIFFS if it exists  
    spiffs_src = build_dir / "spiffs.bin"
    if spiffs_src.exists():
        spiffs_dst = ota_dir / f"antenna_switch_spiffs_{build_time}.bin"
        shutil.copy2(str(spiffs_src), str(spiffs_dst))
        
        # Create/update latest link
        latest_link = ota_dir / "spiffs_latest.bin"
        if latest_link.exists():
            latest_link.unlink()
        shutil.copy2(str(spiffs_src), str(latest_link))
        
        copied_files.append(f"SPIFFS: {spiffs_dst.name}")
        print(f"✅ SPIFFS copied: {spiffs_dst.name}")
    else:
        print(f"⚠️  SPIFFS not found: {spiffs_src}")
    
    print()
    
    if copied_files:
        print("📦 Files ready for OTA upload:")
        for file in copied_files:
            print(f"   {file}")
        print()
        print("🌐 Upload via: http://antenna.local/ota")
        
        # Show file sizes
        print()
        print("📊 File sizes:")
        for file in ota_dir.glob("*.bin"):
            size = file.stat().st_size
            size_str = format_bytes(size)
            print(f"   {file.name}: {size_str}")
    else:
        print("❌ No files were copied. Make sure to build the project first:")
        print("   pio run -e esp32doit-devkit-v1")
        print("   pio run -e esp32doit-devkit-v1 --target buildfs")

def format_bytes(bytes):
    """Format bytes in human readable format"""
    if bytes == 0:
        return "0 B"
    
    units = ['B', 'KB', 'MB', 'GB']
    size = float(bytes)
    unit_index = 0
    
    while size >= 1024 and unit_index < len(units) - 1:
        size /= 1024
        unit_index += 1
    
    if unit_index == 0:
        return f"{int(size)} {units[unit_index]}"
    else:
        return f"{size:.1f} {units[unit_index]}"

if __name__ == "__main__":
    main()