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

void loadSettings() {
  if(SPIFFS.exists("/settings.json")) {
    File file = SPIFFS.open("/settings.json", "r");
    if(file) {
      DynamicJsonDocument doc(2048);
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
      if(doc.containsKey("antennaNames")) {
        JsonArray names = doc["antennaNames"].as<JsonArray>();
        for(int i = 0; i < 6 && i < (int)names.size(); i++) {
          antennaNames[i] = names[i].as<String>();
        }
      }
      if(doc.containsKey("antennaBands")) {
        JsonArray bandsArr = doc["antennaBands"].as<JsonArray>();
        for(int i = 0; i < 6 && i < (int)bandsArr.size(); i++) {
          JsonArray bands = bandsArr[i].as<JsonArray>();
          String joined = "";
          for(int j = 0; j < (int)bands.size(); j++) {
            if(j > 0) joined += ",";
            joined += bands[j].as<String>();
          }
          antennaBands[i] = joined;
        }
      }
    }
  }
}

void saveSettings() {
  DynamicJsonDocument doc(2048);
  doc["mdnsHostname"] = mdnsHostname.c_str();
  doc["antennaSwapping"] = antennaSwappingEnabled;
  doc["singleRadioMode"] = singleRadioMode;
  JsonArray names = doc.createNestedArray("antennaNames");
  for(int i = 0; i < 6; i++) {
    names.add(antennaNames[i]);
  }
  JsonArray bandsArr = doc.createNestedArray("antennaBands");
  for(int i = 0; i < 6; i++) {
    JsonArray bands = bandsArr.createNestedArray();
    String src = antennaBands[i];
    while(src.length() > 0) {
      int idx = src.indexOf(',');
      if(idx == -1) {
        if(src.length() > 0) bands.add(src);
        break;
      }
      bands.add(src.substring(0, idx));
      src = src.substring(idx + 1);
    }
  }

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
