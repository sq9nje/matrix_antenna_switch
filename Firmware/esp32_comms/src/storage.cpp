#include "storage.h"
#include "globals.h"
#include <SPIFFS.h>
#include <ArduinoJson.h>

bool initializeStorage() {
  if(!SPIFFS.begin(true)) {
    Serial.println("SPIFFS Mount Failed");
    return false;
  }
  return true;
}

void loadAntennaNames() {
  if(SPIFFS.exists("/antenna_names.json")) {
    File file = SPIFFS.open("/antenna_names.json", "r");
    if(file) {
      DynamicJsonDocument doc(1024);
      deserializeJson(doc, file);
      file.close();
      
      for(int i = 0; i < 6; i++) {
        if(doc.containsKey(String(i))) {
          antennaNames[i] = doc[String(i)].as<String>();
        }
      }
    }
  }
}

void saveAntennaNames() {
  DynamicJsonDocument doc(1024);
  for(int i = 0; i < 6; i++) {
    doc[String(i)] = antennaNames[i];
  }
  
  File file = SPIFFS.open("/antenna_names.json", "w");
  if(file) {
    serializeJson(doc, file);
    file.close();
  }
}

void loadSettings() {
  if(SPIFFS.exists("/settings.json")) {
    File file = SPIFFS.open("/settings.json", "r");
    if(file) {
      DynamicJsonDocument doc(1024);
      deserializeJson(doc, file);
      file.close();
      
      if(doc.containsKey("mdnsHostname")) {
        const char* hostname = doc["mdnsHostname"];
        if(hostname) {
          mdnsHostname = String(hostname);
        }
      }
      if(doc.containsKey("antennaSwapping")) {
        antennaSwappingEnabled = doc["antennaSwapping"].as<bool>();
      }
      if(doc.containsKey("singleRadioMode")) {
        singleRadioMode = doc["singleRadioMode"].as<bool>();
      }
    }
  }
}

void saveSettings() {
  DynamicJsonDocument doc(1024);
  doc["mdnsHostname"] = mdnsHostname.c_str();
  doc["antennaSwapping"] = antennaSwappingEnabled;
  doc["singleRadioMode"] = singleRadioMode;
  
  File file = SPIFFS.open("/settings.json", "w");
  if(file) {
    serializeJson(doc, file);
    file.close();
  }
}

String validateHostname(const String& input) {
  // Check length constraints
  if(input.length() == 0 || input.length() > 63) {
    return ""; // Empty string indicates invalid
  }
  
  // Remove invalid characters and convert to lowercase
  String validHostname = "";
  for(int i = 0; i < input.length(); i++) {
    char c = input[i];
    if((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '-') {
      validHostname += c;
    }
  }
  validHostname.toLowerCase();
  
  // Return empty string if no valid characters remain
  return validHostname.length() > 0 ? validHostname : "";
}
