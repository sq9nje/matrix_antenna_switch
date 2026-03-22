#include "web_server.h"
#include "globals.h"
#include "storage.h"
#include "websocket.h"
#include "antenna_hardware.h"
#include "wifi_manager.h"
#include "otrsp.h"
#include <WiFi.h>
#include <ESPmDNS.h>
#include <SPIFFS.h>
#include <ArduinoJson.h>
#include <Update.h>

void initializeMDNS() {
  if (MDNS.begin(mdnsHostname.c_str())) {
    Serial.println("mDNS responder started");
    Serial.print("You can now connect to http://");
    Serial.print(mdnsHostname);
    Serial.println(".local");
    
    // Add service to mDNS
    MDNS.addService("http", "tcp", 80);
    MDNS.addService("ws", "tcp", 81);
    MDNS.addService("otrsp", "tcp", OTRSP_TCP_PORT);
  } else {
    Serial.println("Error setting up mDNS responder!");
  }
}

void initializeWebServer() {
  // Static file routes
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/index.html", "text/html");
  });

  server.on("/settings", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/settings.html", "text/html");
  });

  server.on("/style.css", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/style.css", "text/css");
  });

  server.on("/script.js", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/script.js", "text/javascript");
  });

  server.on("/status", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/status.html", "text/html");
  });

  server.on("/ota", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/ota.html", "text/html");
  });

  // Antenna management API
  server.on("/api/antennas", HTTP_GET, [](AsyncWebServerRequest *request){
    DynamicJsonDocument doc(2048);
    JsonArray array = doc.to<JsonArray>();
    for(int i = 0; i < 6; i++) {
      JsonObject ant = array.createNestedObject();
      ant["name"] = antennas[i].name;
      JsonArray bands = ant.createNestedArray("bands");
      for(const auto& band : antennas[i].bands) {
        bands.add(band);
      }
    }
    String response;
    serializeJson(doc, response);
    request->send(200, "application/json", response);
  });

  server.on("/api/antennas", HTTP_POST, [](AsyncWebServerRequest *request){}, NULL,
    [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total){
      DynamicJsonDocument doc(2048);
      deserializeJson(doc, (char*)data);

      for(int i = 0; i < 6; i++) {
        String key = String(i);
        if(doc.containsKey(key)) {
          JsonVariant val = doc[key];
          if(val.is<JsonObject>()) {
            JsonObject obj = val.as<JsonObject>();
            if(obj.containsKey("name")) {
              antennas[i].name = obj["name"].as<String>();
            }
            if(obj.containsKey("bands")) {
              antennas[i].bands.clear();
              JsonArray bands = obj["bands"].as<JsonArray>();
              for(int j = 0; j < (int)bands.size(); j++) {
                antennas[i].bands.push_back(bands[j].as<String>());
              }
            }
          } else {
            // Backward compatibility: plain string value = name only
            antennas[i].name = val.as<String>();
          }
        }
      }

      saveSettings();
      sendAntennaNameUpdate();
      request->send(200, "text/plain", "OK");
    });

  // Individual antenna name management
  server.on("^\\/api\\/antenna\\/(\\d+)$", HTTP_GET, [](AsyncWebServerRequest *request){
    String antennaStr = request->pathArg(0);
    int antennaIndex = antennaStr.toInt();

    if(antennaIndex >= 0 && antennaIndex < 6) {
      DynamicJsonDocument doc(512);
      doc["index"] = antennaIndex;
      doc["name"] = antennas[antennaIndex].name;
      JsonArray bands = doc.createNestedArray("bands");
      for(const auto& band : antennas[antennaIndex].bands) {
        bands.add(band);
      }

      String response;
      serializeJson(doc, response);
      request->send(200, "application/json", response);
    } else {
      request->send(400, "text/plain", "Invalid antenna index");
    }
  });

  server.on("^\\/api\\/antenna\\/(\\d+)$", HTTP_PUT, [](AsyncWebServerRequest *request){}, NULL,
    [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total){
      String antennaStr = request->pathArg(0);
      int antennaIndex = antennaStr.toInt();

      if(antennaIndex >= 0 && antennaIndex < 6) {
        DynamicJsonDocument doc(512);
        deserializeJson(doc, (char*)data);

        bool updated = false;
        if(doc.containsKey("name")) {
          antennas[antennaIndex].name = doc["name"].as<String>();
          updated = true;
        }
        if(doc.containsKey("bands")) {
          antennas[antennaIndex].bands.clear();
          JsonArray bands = doc["bands"].as<JsonArray>();
          for(int j = 0; j < (int)bands.size(); j++) {
            antennas[antennaIndex].bands.push_back(bands[j].as<String>());
          }
          updated = true;
        }

        if(updated) {
          saveSettings();
          sendAntennaNameUpdate();
          request->send(200, "text/plain", "OK");
        } else {
          request->send(400, "text/plain", "Missing 'name' or 'bands' field");
        }
      } else {
        request->send(400, "text/plain", "Invalid antenna index");
      }
    });

  // State API
  server.on("/api/state", HTTP_GET, [](AsyncWebServerRequest *request){
    DynamicJsonDocument doc(200);
    doc["radio1"] = currentAntenna[0];
    doc["radio2"] = currentAntenna[1];
    String response;
    serializeJson(doc, response);
    request->send(200, "application/json", response);
  });

  // mDNS hostname management
  server.on("/api/hostname", HTTP_GET, [](AsyncWebServerRequest *request){
    String response = "{\"hostname\":\"" + mdnsHostname + "\"}";
    request->send(200, "application/json", response);
  });

  server.on("/api/hostname", HTTP_POST, [](AsyncWebServerRequest *request){}, NULL, 
    [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total){
      DynamicJsonDocument doc(256);
      deserializeJson(doc, (char*)data);
      
      if(doc.containsKey("hostname")) {
        String newHostname = doc["hostname"].as<String>();
        String validHostname = validateHostname(newHostname);
        
        if(validHostname.length() > 0) {
          mdnsHostname = validHostname;
          saveSettings();
          
          request->send(200, "text/plain", "OK - Restart required for changes to take effect");
        } else {
          request->send(400, "text/plain", "Invalid hostname format or length (1-63 characters, letters/numbers/hyphens only)");
        }
      } else {
        request->send(400, "text/plain", "Missing 'hostname' field");
      }
    });

  // Operation mode settings
  server.on("/api/operation-mode", HTTP_GET, [](AsyncWebServerRequest *request){
    DynamicJsonDocument doc(256);
    doc["antennaSwapping"] = antennaSwappingEnabled;
    doc["singleRadioMode"] = singleRadioMode;
    String response;
    serializeJson(doc, response);
    request->send(200, "application/json", response);
  });

  server.on("/api/operation-mode", HTTP_POST, [](AsyncWebServerRequest *request){}, NULL, 
    [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total){
      DynamicJsonDocument doc(256);
      deserializeJson(doc, (char*)data);
      
      if(doc.containsKey("antennaSwapping")) {
        antennaSwappingEnabled = doc["antennaSwapping"].as<bool>();
      }
      
      if(doc.containsKey("singleRadioMode")) {
        bool newSingleRadioMode = doc["singleRadioMode"].as<bool>();
        
        // If enabling single radio mode, disconnect radio 2
        if(newSingleRadioMode && !singleRadioMode) {
          if(currentAntenna[1] > 0) {
            digitalWrite(relay[1][currentAntenna[1]-1], 0);
            currentAntenna[1] = 0;
          }
        }
        
        singleRadioMode = newSingleRadioMode;
      }
      
      saveSettings();
      sendWebSocketUpdate(); // Update the UI with new state
      request->send(200, "text/plain", "OK");
    });

  // Admin endpoints
  server.on("/api/reboot", HTTP_POST, [](AsyncWebServerRequest *request){
    request->send(200, "text/plain", "Device rebooting...");
    delay(1000);
    ESP.restart();
  });

  server.on("/api/reset-network", HTTP_POST, [](AsyncWebServerRequest *request){
    request->send(200, "text/plain", "Network settings reset. Device will reboot...");
    resetNetworkSettings();
  });

  // Settings export
  server.on("/api/settings/export", HTTP_GET, [](AsyncWebServerRequest *request){
    DynamicJsonDocument doc(2048);
    doc["mdnsHostname"] = mdnsHostname.c_str();
    doc["antennaSwapping"] = antennaSwappingEnabled;
    doc["singleRadioMode"] = singleRadioMode;
    doc["otrspEnabled"] = otrspEnabled;
    doc["otrspSerialEnabled"] = otrspSerialEnabled;
    JsonArray arr = doc.createNestedArray("antennas");
    for(int i = 0; i < 6; i++) {
      JsonObject obj = arr.createNestedObject();
      obj["name"] = antennas[i].name;
      JsonArray bands = obj.createNestedArray("bands");
      for(const auto& band : antennas[i].bands) {
        bands.add(band);
      }
    }

    String response;
    serializeJsonPretty(doc, response);
    AsyncWebServerResponse *resp = request->beginResponse(200, "application/json", response);
    resp->addHeader("Content-Disposition", "attachment; filename=\"settings.json\"");
    request->send(resp);
  });

  // Settings import
  server.on("/api/settings/import", HTTP_POST, [](AsyncWebServerRequest *request){}, NULL,
    [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total){
      DynamicJsonDocument doc(2048);
      DeserializationError error = deserializeJson(doc, (char*)data);

      if(error) {
        request->send(400, "text/plain", "Invalid JSON");
        return;
      }

      if(doc.containsKey("mdnsHostname")) {
        String newHostname = validateHostname(doc["mdnsHostname"].as<String>());
        if(newHostname.length() > 0) {
          mdnsHostname = newHostname;
        }
      }
      if(doc.containsKey("antennaSwapping")) {
        antennaSwappingEnabled = doc["antennaSwapping"].as<bool>();
      }
      if(doc.containsKey("singleRadioMode")) {
        bool newSingleRadioMode = doc["singleRadioMode"].as<bool>();
        if(newSingleRadioMode && !singleRadioMode) {
          if(currentAntenna[1] > 0) {
            digitalWrite(relay[1][currentAntenna[1]-1], 0);
            currentAntenna[1] = 0;
          }
        }
        singleRadioMode = newSingleRadioMode;
      }
      if(doc.containsKey("otrspEnabled")) {
        otrspEnabled = doc["otrspEnabled"].as<bool>();
      }
      if(doc.containsKey("otrspSerialEnabled")) {
        otrspSerialEnabled = doc["otrspSerialEnabled"].as<bool>();
      }
      if(doc.containsKey("antennas")) {
        // New format: array of objects
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
        // Old format: separate arrays
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

      saveSettings();
      sendWebSocketUpdate();
      sendAntennaNameUpdate();
      request->send(200, "text/plain", "Settings imported successfully");
    });

  // Status API
  server.on("/api/status", HTTP_GET, [](AsyncWebServerRequest *request){
    DynamicJsonDocument doc(1024);
    
    // WiFi information
    doc["ssid"] = WiFi.SSID();
    doc["ip"] = WiFi.localIP().toString();
    doc["gateway"] = WiFi.gatewayIP().toString();
    doc["subnet"] = WiFi.subnetMask().toString();
    doc["dns"] = WiFi.dnsIP().toString();
    doc["rssi"] = WiFi.RSSI();
    doc["macAddress"] = WiFi.macAddress();
    doc["hostname"] = mdnsHostname;
    
    // Firmware information
    #ifdef FIRMWARE_VERSION
    doc["firmwareVersion"] = FIRMWARE_VERSION;
    #else
    doc["firmwareVersion"] = "Unknown";
    #endif
    
    #ifdef BUILD_TIME
    doc["buildTime"] = BUILD_TIME;
    #else
    doc["buildTime"] = "Unknown";
    #endif

    // Device information
    doc["chipModel"] = ESP.getChipModel();
    doc["chipRevision"] = ESP.getChipRevision();
    doc["cpuFreqMHz"] = ESP.getCpuFreqMHz();
    doc["freeHeap"] = ESP.getFreeHeap();
    doc["totalHeap"] = ESP.getHeapSize();
    doc["uptime"] = millis() / 1000;
    
    // Current antenna state
    doc["currentRadio1"] = currentAntenna[0];
    doc["currentRadio2"] = currentAntenna[1];
    
    String response;
    serializeJson(doc, response);
    request->send(200, "application/json", response);
  });

  // OTA Update endpoint
  server.on("/api/update", HTTP_POST, 
    [](AsyncWebServerRequest *request) {
      bool updateHasError = Update.hasError();
      String message = updateHasError ? "Update Failed" : "Update Success";
      request->send(200, "text/plain", message);
      
      if (!updateHasError) {
        delay(1000);
        ESP.restart();
      }
    },
    [](AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final) {
      if (!index) {
        Serial.printf("Update Start: %s\n", filename.c_str());
        
        // Turn off all relays during update
        for(uint8_t radio = 0; radio < 2; radio++) {
          for(uint8_t antenna = 0; antenna < 6; antenna++) {
            digitalWrite(relay[radio][antenna], LOW);
          }
        }
        currentAntenna[0] = 0;
        currentAntenna[1] = 0;
        
        // Determine update type based on filename
        int cmd;
        if (filename.indexOf("spiffs") >= 0 || filename.indexOf("SPIFFS") >= 0) {
          cmd = U_SPIFFS;
          Serial.println("Detected SPIFFS update");
        } else if (filename.indexOf("firmware") >= 0 || filename.indexOf("FIRMWARE") >= 0) {
          cmd = U_FLASH;
          Serial.println("Detected firmware update");
        } else if (filename.endsWith(".bin")) {
          // Default: assume firmware for generic .bin files
          cmd = U_FLASH;
          Serial.println("Generic .bin file - assuming firmware update");
        } else {
          Serial.println("Unknown file type - assuming SPIFFS");
          cmd = U_SPIFFS;
        }
        
        if (!Update.begin(UPDATE_SIZE_UNKNOWN, cmd)) {
          Update.printError(Serial);
          sendOTAStatus("error", "Failed to begin update", 0);
          return;
        }
        
        sendOTAStatus("starting", filename, 0);
      }

      if (len) {
        if (Update.write(data, len) != len) {
          Update.printError(Serial);
          sendOTAStatus("error", "Write failed", 0);
          return;
        }
        
        // Send progress update
        size_t progress = (index + len) * 100 / request->contentLength();
        sendOTAStatus("progress", "", progress);
      }

      if (final) {
        if (Update.end(true)) {
          Serial.printf("Update Success: %uB\n", index + len);
          sendOTAStatus("complete", "Update successful", 100);
        } else {
          Update.printError(Serial);
          sendOTAStatus("error", "Update failed to complete", 0);
        }
      }
    }
  );

  // OTRSP status API
  server.on("/api/otrsp/status", HTTP_GET, [](AsyncWebServerRequest *request){
    DynamicJsonDocument doc(512);
    doc["enabled"] = otrspEnabled;
    doc["serialEnabled"] = otrspSerialEnabled;
    doc["tcpPort"] = OTRSP_TCP_PORT;
    doc["clientConnected"] = otrspState.clientConnected;
    doc["txFocus"] = otrspState.txFocus;
    doc["rxFocus"] = otrspState.rxFocus;
    doc["band1"] = otrspState.band[0];
    doc["band2"] = otrspState.band[1];
    doc["mode1"] = String(otrspState.mode[0]);
    doc["mode2"] = String(otrspState.mode[1]);
    String response;
    serializeJson(doc, response);
    request->send(200, "application/json", response);
  });

  // OTRSP enable/disable
  server.on("/api/otrsp/enable", HTTP_POST, [](AsyncWebServerRequest *request){}, NULL,
    [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total){
      DynamicJsonDocument doc(256);
      deserializeJson(doc, (char*)data);

      if(doc.containsKey("enabled")) {
        otrspEnabled = doc["enabled"].as<bool>();
      }
      if(doc.containsKey("serialEnabled")) {
        otrspSerialEnabled = doc["serialEnabled"].as<bool>();
      }
      saveSettings();
      request->send(200, "text/plain", "OK - Restart required for TCP changes to take effect");
    });

  server.begin();
  Serial.println("HTTP server started");
}
