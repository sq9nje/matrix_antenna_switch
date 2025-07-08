#include <Arduino.h>
#include <WiFi.h>
#include <WiFiManager.h>
#include <ESPAsyncWebServer.h>
#include <WebSocketsServer.h>
#include <ArduinoJson.h>
#include <SPIFFS.h>
#include <ESPmDNS.h>

// Project includes
#include "globals.h"
#include "antenna_hardware.h"
#include "websocket.h"
#include "command_parser.h"
#include "storage.h"
#include "web_server.h"
#include "wifi_manager.h"

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

  // Initialize and start web server
  initializeWebServer();

  blink(3);
  Serial.println("System ready");
}

void loop() {
  webSocket.loop();
  
  // Handle UART0 (Serial) commands - debug output
  handleSerialInput(Serial, Serial);
  
  // Handle UART2 commands - debug output to Serial
  handleSerialInput(Serial2, Serial);
}
