#include "websocket.h"
#include "globals.h"
#include "antenna_hardware.h"

void sendWebSocketUpdate() {
  DynamicJsonDocument doc(300);
  doc["type"] = "state";
  doc["radio1"] = currentAntenna[0];
  doc["radio2"] = currentAntenna[1];
  doc["singleRadioMode"] = singleRadioMode;
  
  String message;
  serializeJson(doc, message);
  webSocket.broadcastTXT(message);
}

void sendAntennaNameUpdate() {
  DynamicJsonDocument doc(2048);
  doc["type"] = "antennaNames";
  JsonArray antennas = doc.createNestedArray("antennas");
  for(int i = 0; i < 6; i++) {
    JsonObject ant = antennas.createNestedObject();
    ant["name"] = antennaNames[i];
    JsonArray bands = ant.createNestedArray("bands");
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

  String message;
  serializeJson(doc, message);
  webSocket.broadcastTXT(message);
}

void sendOTAStatus(const String& status, const String& message, uint8_t progress) {
  DynamicJsonDocument doc(300);
  doc["type"] = "ota";
  doc["status"] = status;
  doc["message"] = message;
  doc["progress"] = progress;
  
  String jsonMessage;
  serializeJson(doc, jsonMessage);
  webSocket.broadcastTXT(jsonMessage);
}

void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length) {
  switch(type) {
    case WStype_DISCONNECTED:
      Serial.printf("[%u] Disconnected!\n", num);
      break;
      
    case WStype_CONNECTED:
      Serial.printf("[%u] Connected from %s\n", num, webSocket.remoteIP(num).toString().c_str());
      sendWebSocketUpdate(); // Send current state to new client
      sendAntennaNameUpdate(); // Send antenna names to new client
      break;
      
    case WStype_TEXT:
      {
        String message = String((char*)payload);
        DynamicJsonDocument doc(200);
        deserializeJson(doc, message);
        
        if(doc["type"] == "select") {
          uint8_t radio = doc["radio"];
          uint8_t antenna = doc["antenna"];
          selectAntenna(radio, antenna);
        }
      }
      break;
      
    default:
      break;
  }
}

void initializeWebSocket() {
  webSocket.begin();
  webSocket.onEvent(webSocketEvent);
}
