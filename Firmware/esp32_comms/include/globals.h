#ifndef GLOBALS_H
#define GLOBALS_H

#include <Arduino.h>

// Hardware pin definitions
#define STATUS_LED  32
#define RXD2        16
#define TXD2        17
#define BUF_SIZE    32

// Forward declarations
class AsyncWebServer;
class WebSocketsServer;

// Global variables
extern uint8_t currentAntenna[2];
extern const uint8_t relay[2][6];
extern String antennaNames[6];
extern String mdnsHostname;
extern bool antennaSwappingEnabled;
extern bool singleRadioMode;

// Global objects
extern AsyncWebServer server;
extern WebSocketsServer webSocket;

#endif
