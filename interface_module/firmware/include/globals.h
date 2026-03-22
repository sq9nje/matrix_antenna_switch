#ifndef GLOBALS_H
#define GLOBALS_H

#include <Arduino.h>
#include <vector>

// Hardware pin definitions
#define STATUS_LED  33
#define RXD2        16
#define TXD2        17
#define BUF_SIZE    32

// Forward declarations
class AsyncWebServer;
class WebSocketsServer;

// Antenna configuration
struct AntennaConfig {
    String name;
    std::vector<String> bands;  // empty = no bands selected
};

// Global variables
extern uint8_t currentAntenna[2];
extern const uint8_t relay[2][6];
extern AntennaConfig antennas[6];
extern String mdnsHostname;
extern bool antennaSwappingEnabled;
extern bool singleRadioMode;
extern bool otrspEnabled;
extern bool otrspSerialEnabled;

// Global objects
extern AsyncWebServer server;
extern WebSocketsServer webSocket;

#endif
