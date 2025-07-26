#include "globals.h"
#include <ESPAsyncWebServer.h>
#include <WebSocketsServer.h>

// Global variables definitions
uint8_t currentAntenna[2] = {0, 0}; // 0 means disconnected
const uint8_t relay[2][6] = {{25, 26, 27, 14, 12, 13}, {5, 5, 5, 4, 4, 4}};
String antennaNames[6] = {"Antenna 1", "Antenna 2", "Antenna 3", "Antenna 4", "Antenna 5", "Antenna 6"};
String mdnsHostname = "antenna";
bool antennaSwappingEnabled = false;
bool singleRadioMode = false;

// Global objects
AsyncWebServer server(80);
WebSocketsServer webSocket = WebSocketsServer(81);
