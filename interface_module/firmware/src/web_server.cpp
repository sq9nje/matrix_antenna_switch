#include "web_server.h"
#include "globals.h"
#include "storage.h"
#include "websocket.h"
#include "antenna_hardware.h"
#include "wifi_manager.h"
#include <WiFi.h>
#include <ESPmDNS.h>
#include <SPIFFS.h>
#include <ArduinoJson.h>

void initializeMDNS() {
  if (MDNS.begin(mdnsHostname.c_str())) {
    Serial.println("mDNS responder started");
    Serial.print("You can now connect to http://");
    Serial.print(mdnsHostname);
    Serial.println(".local");
    
    // Add service to mDNS
    MDNS.addService("http", "tcp", 80);
    MDNS.addService("ws", "tcp", 81);
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

  // Antenna management API
  server.on("/api/antennas", HTTP_GET, [](AsyncWebServerRequest *request){
    DynamicJsonDocument doc(1024);
    JsonArray array = doc.to<JsonArray>();
    for(int i = 0; i < 6; i++) {
      array.add(antennaNames[i]);
    }
    String response;
    serializeJson(doc, response);
    request->send(200, "application/json", response);
  });

  server.on("/api/antennas", HTTP_POST, [](AsyncWebServerRequest *request){}, NULL, 
    [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total){
      DynamicJsonDocument doc(1024);
      deserializeJson(doc, (char*)data);
      
      for(int i = 0; i < 6; i++) {
        if(doc.containsKey(String(i))) {
          antennaNames[i] = doc[String(i)].as<String>();
        }
      }
      
      saveAntennaNames();
      sendAntennaNameUpdate(); // Broadcast updated names via WebSocket
      request->send(200, "text/plain", "OK");
    });

  // Individual antenna name management
  server.on("^\\/api\\/antenna\\/(\\d+)$", HTTP_GET, [](AsyncWebServerRequest *request){
    String antennaStr = request->pathArg(0);
    int antennaIndex = antennaStr.toInt();
    
    if(antennaIndex >= 0 && antennaIndex < 6) {
      DynamicJsonDocument doc(256);
      doc["index"] = antennaIndex;
      doc["name"] = antennaNames[antennaIndex];
      
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
        DynamicJsonDocument doc(256);
        deserializeJson(doc, (char*)data);
        
        if(doc.containsKey("name")) {
          antennaNames[antennaIndex] = doc["name"].as<String>();
          saveAntennaNames();
          sendAntennaNameUpdate(); // Broadcast updated names via WebSocket
          request->send(200, "text/plain", "OK");
        } else {
          request->send(400, "text/plain", "Missing 'name' field");
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

  server.begin();
  Serial.println("HTTP server started");
}
