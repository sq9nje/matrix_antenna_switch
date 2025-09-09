#include <Arduino.h>
#include <WiFi.h>
#include <WiFiManager.h>
#include <ESPAsyncWebServer.h>
#include <WebSocketsServer.h>
#include <ArduinoJson.h>
#include <SPIFFS.h>
#include <ESPmDNS.h>
#include <ArduinoOTA.h>

// Project includes
#include "globals.h"
#include "antenna_hardware.h"
#include "websocket.h"
#include "command_parser.h"
#include "storage.h"
#include "web_server.h"
#include "wifi_manager.h"

void initializeOTA() {
  ArduinoOTA.setHostname(mdnsHostname.c_str());
  ArduinoOTA.setPassword("antenna123");  // Set OTA password for security
  
  ArduinoOTA.onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH) {
      type = "sketch";
    } else { // U_SPIFFS
      type = "filesystem";
      SPIFFS.end();
    }
    Serial.println("Start updating " + type);
    
    // Turn off all relays during OTA
    for(uint8_t radio = 0; radio < 2; radio++) {
      for(uint8_t antenna = 0; antenna < 6; antenna++) {
        digitalWrite(relay[radio][antenna], LOW);
      }
    }
    currentAntenna[0] = 0;
    currentAntenna[1] = 0;
    
    // Notify connected clients
    sendOTAStatus("starting", type, 0);
  });
  
  ArduinoOTA.onEnd([]() {
    Serial.println("\nOTA Update complete");
    sendOTAStatus("complete", "", 100);
    delay(1000);
  });
  
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    unsigned int percent = (progress / (total / 100));
    Serial.printf("Progress: %u%%\r", percent);
    sendOTAStatus("progress", "", percent);
  });
  
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    String errorMsg = "";
    if (error == OTA_AUTH_ERROR) {
      errorMsg = "Auth Failed";
      Serial.println("Auth Failed");
    } else if (error == OTA_BEGIN_ERROR) {
      errorMsg = "Begin Failed";
      Serial.println("Begin Failed");
    } else if (error == OTA_CONNECT_ERROR) {
      errorMsg = "Connect Failed";
      Serial.println("Connect Failed");
    } else if (error == OTA_RECEIVE_ERROR) {
      errorMsg = "Receive Failed";
      Serial.println("Receive Failed");
    } else if (error == OTA_END_ERROR) {
      errorMsg = "End Failed";
      Serial.println("End Failed");
    }
    sendOTAStatus("error", errorMsg, 0);
  });
  
  ArduinoOTA.begin();
  Serial.println("OTA Ready");
  Serial.print("OTA Password: antenna123");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}

void setup() {
  Serial.begin(115200);
  Serial2.begin(9600, SERIAL_8N1, RXD2, TXD2);
  
  Serial.println("Starting 6x2 Antenna Switch SQ9NJE");

  // Initialize storage
  if(!initializeStorage()) {
    return;
  }

  // Initialize hardware
  initializeHardware();

  // Load settings and antenna names
  loadSettings();
  loadAntennaNames();

  // Initialize network
  initializeWiFi();
  initializeMDNS();

  // Initialize WebSocket server
  initializeWebSocket();

  // Initialize OTA
  initializeOTA();

  // Initialize and start web server
  initializeWebServer();

  blink(3);
  Serial.println("System ready");
}

void loop() {
  ArduinoOTA.handle();
  webSocket.loop();
  
  // Handle UART0 (Serial) commands - debug output
  handleSerialInput(Serial, Serial);
  
  // Handle UART2 commands - debug output to Serial
  handleSerialInput(Serial2, Serial);
}
