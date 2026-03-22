#ifndef OTRSP_H
#define OTRSP_H

#include <Arduino.h>
#include <WiFiServer.h>
#include <WiFiClient.h>

#define OTRSP_TCP_PORT 12060
#define OTRSP_BUF_SIZE 40

struct OTRSPState {
    uint8_t txFocus;       // 1 or 2
    String rxFocus;        // "1", "2", "1S", "2S", "1R", "2R"
    String band[2];        // frequency string per radio
    char mode[2];          // mode char per radio (C/U/L/R/F/A/X/0)
    bool clientConnected;
};

extern OTRSPState otrspState;

void initializeOTRSP();
void handleOTRSPLoop();
void parseOTRSPCommand(const char* cmd, Stream& response);
void handleOTRSPSerialInput(Stream& serial);

#endif
