#include "globals.h"
#include <ESPAsyncWebServer.h>
#include <WebSocketsServer.h>

// Global variables definitions
uint8_t currentAntenna[2] = {0, 0}; // 0 means disconnected
const uint8_t relay[2][6] = {{13, 12, 14, 27, 26, 25}, {5, 18, 19, 21, 22, 23}};
AntennaConfig antennas[6] = {
    {"Antenna 1", {}}, {"Antenna 2", {}}, {"Antenna 3", {}},
    {"Antenna 4", {}}, {"Antenna 5", {}}, {"Antenna 6", {}}
};
String mdnsHostname = "antenna";
bool antennaSwappingEnabled = false;
bool singleRadioMode = false;
bool otrspEnabled = false;
bool otrspSerialEnabled = false;

// Global objects
AsyncWebServer server(80);
WebSocketsServer webSocket = WebSocketsServer(81);
