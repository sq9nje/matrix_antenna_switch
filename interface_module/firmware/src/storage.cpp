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
      if(doc.containsKey("antennas")) {
        // New format: array of objects with name and bands
        JsonArray arr = doc["antennas"].as<JsonArray>();
        for(int i = 0; i < 6 && i < (int)arr.size(); i++) {
          JsonObject obj = arr[i].as<JsonObject>();
          if(obj.containsKey("name")) {
            antennas[i].name = obj["name"].as<String>();
          }
          antennas[i].bands.clear();
          if(obj.containsKey("bands")) {
            JsonArray bands = obj["bands"].as<JsonArray>();
            for(int j = 0; j < (int)bands.size(); j++) {
              antennas[i].bands.push_back(bands[j].as<String>());
            }
          }
        }
      } else {
        // Old format: separate antennaNames and antennaBands arrays
        if(doc.containsKey("antennaNames")) {
          JsonArray names = doc["antennaNames"].as<JsonArray>();
          for(int i = 0; i < 6 && i < (int)names.size(); i++) {
            antennas[i].name = names[i].as<String>();
          }
        }
        if(doc.containsKey("antennaBands")) {
          JsonArray bandsArr = doc["antennaBands"].as<JsonArray>();
          for(int i = 0; i < 6 && i < (int)bandsArr.size(); i++) {
            antennas[i].bands.clear();
            JsonArray bands = bandsArr[i].as<JsonArray>();
            for(int j = 0; j < (int)bands.size(); j++) {
              antennas[i].bands.push_back(bands[j].as<String>());
            }
          }
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
  JsonArray arr = doc.createNestedArray("antennas");
  for(int i = 0; i < 6; i++) {
    JsonObject obj = arr.createNestedObject();
    obj["name"] = antennas[i].name;
    JsonArray bands = obj.createNestedArray("bands");
    for(const auto& band : antennas[i].bands) {
      bands.add(band);
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
