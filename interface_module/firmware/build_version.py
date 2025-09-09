#!/usr/bin/env python3

"""
Build version script for ESP32 Antenna Switch
Adds version information to the build
"""

Import("env")
import datetime

# Get current timestamp
build_time = datetime.datetime.now().strftime("%Y%m%d_%H%M%S")

# Add build flags
env.Append(CPPDEFINES=[
    ("BUILD_TIME", f'\\"{build_time}\\"'),
    ("FIRMWARE_VERSION", '\\"1.0.0\\"')
])

print(f"🔧 Build script loaded - firmware version 1.0.0 at {build_time}")
print(f"📁 Project directory: {env.subst('$PROJECT_DIR')}")
print(f"📁 Build directory: {env.subst('$BUILD_DIR')}")

def copy_firmware_after_build(source, target, env):
    """Copy firmware to OTA directory - simplified version"""
    import os
    import shutil
    
    print("🔧 Post-build: Copying firmware to OTA directory")
    
    # Create ota_builds directory
    project_dir = env.subst("$PROJECT_DIR")
    ota_dir = os.path.join(project_dir, "ota_builds")
    os.makedirs(ota_dir, exist_ok=True)
    
    # Copy firmware.bin
    build_dir = env.subst("$BUILD_DIR")
    firmware_src = os.path.join(build_dir, "firmware.bin")
    
    if os.path.exists(firmware_src):
        # Create timestamped filename
        firmware_dst = os.path.join(ota_dir, f"antenna_switch_firmware_{build_time}.bin")
        shutil.copy2(firmware_src, firmware_dst)
        
        # Create latest link (simple copy for compatibility)
        latest_link = os.path.join(ota_dir, "firmware_latest.bin")
        if os.path.exists(latest_link):
            os.remove(latest_link)
        shutil.copy2(firmware_src, latest_link)
        
        print(f"✅ Firmware copied: {os.path.basename(firmware_dst)}")
        print(f"✅ Latest link updated: firmware_latest.bin")
    else:
        print(f"❌ Firmware not found at: {firmware_src}")

# Simple approach: Hook into the program building target
print("🔧 Setting up post-build hook using AddPostAction on 'buildprog' target...")

def copy_after_program_build(source, target, env):
    """Copy OTA files after program is built"""
    print("🔧 Post-build hook triggered - copying OTA files...")
    copy_firmware_after_build(source, target, env)

# Hook into the program building step
env.AddPostAction("buildprog", copy_after_program_build)

# Add a custom target for manual copying
def manual_copy_ota():
    """Manual copy function"""
    import os
    import shutil
    
    build_dir = env.subst("$BUILD_DIR")
    project_dir = env.subst("$PROJECT_DIR")
    ota_dir = os.path.join(project_dir, "ota_builds")
    current_time = datetime.datetime.now().strftime("%Y%m%d_%H%M%S")
    
    os.makedirs(ota_dir, exist_ok=True)
    
    # Copy firmware if it exists
    firmware_src = os.path.join(build_dir, "firmware.bin")
    if os.path.exists(firmware_src):
        firmware_dst = os.path.join(ota_dir, f"antenna_switch_firmware_{current_time}.bin")
        shutil.copy2(firmware_src, firmware_dst)
        
        latest_link = os.path.join(ota_dir, "firmware_latest.bin")
        if os.path.exists(latest_link):
            os.remove(latest_link)
        shutil.copy2(firmware_src, latest_link)
        
        print(f"✅ Firmware copied manually: {os.path.basename(firmware_dst)}")
    
    # Copy SPIFFS if it exists  
    spiffs_src = os.path.join(build_dir, "spiffs.bin")
    if os.path.exists(spiffs_src):
        spiffs_dst = os.path.join(ota_dir, f"antenna_switch_spiffs_{current_time}.bin")
        shutil.copy2(spiffs_src, spiffs_dst)
        
        latest_link = os.path.join(ota_dir, "spiffs_latest.bin") 
        if os.path.exists(latest_link):
            os.remove(latest_link)
        shutil.copy2(spiffs_src, latest_link)
        
        print(f"✅ SPIFFS copied manually: {os.path.basename(spiffs_dst)}")

# Create custom target
env.AlwaysBuild(env.Alias("copy_ota", [], manual_copy_ota))