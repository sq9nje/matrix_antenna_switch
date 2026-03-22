#include "otrsp.h"
#include "globals.h"
#include "antenna_hardware.h"

OTRSPState otrspState = {1, "1", {"0", "0"}, {'0', '0'}, false};

static WiFiServer otrspServer(OTRSP_TCP_PORT);
static WiFiClient otrspClient;
static char tcpBuffer[OTRSP_BUF_SIZE];
static uint8_t tcpBufLen = 0;
static char serialBuffer[OTRSP_BUF_SIZE];
static uint8_t serialBufLen = 0;

// Forward declarations
static bool startsWith(const char* str, const char* prefix);
static int prefixLen(const char* str, const char* prefix);

static bool startsWith(const char* str, const char* prefix) {
    return strncmp(str, prefix, strlen(prefix)) == 0;
}

static int prefixLen(const char* str, const char* prefix) {
    size_t len = strlen(prefix);
    if (strncmp(str, prefix, len) == 0) return len;
    return 0;
}

void parseOTRSPCommand(const char* cmd, Stream& response) {
    if (cmd[0] == '\0') return;

    bool isQuery = (cmd[0] == '?');
    const char* body = isQuery ? cmd + 1 : cmd;

    // Ping: just "?" with nothing after
    if (isQuery && body[0] == '\0') {
        response.print("?\r");
        return;
    }

    int plen;

    // TX command
    if ((plen = prefixLen(body, "TX")) > 0) {
        const char* rest = body + plen;
        if (isQuery) {
            response.printf("TX%u\r", otrspState.txFocus);
        } else if (rest[0] == '1' || rest[0] == '2') {
            otrspState.txFocus = rest[0] - '0';
        }
        return;
    }

    // RX command
    if ((plen = prefixLen(body, "RX")) > 0) {
        const char* rest = body + plen;
        if (isQuery) {
            response.printf("RX%s\r", otrspState.rxFocus.c_str());
        } else if (rest[0] != '\0') {
            otrspState.rxFocus = String(rest);
        }
        return;
    }

    // AUX command
    if ((plen = prefixLen(body, "AUX")) > 0) {
        const char* rest = body + plen;
        // Item is the port number (1 or 2)
        if (rest[0] >= '1' && rest[0] <= '2') {
            uint8_t radio = rest[0] - '1'; // 0 or 1
            const char* valStr = rest + 1;
            if (isQuery) {
                response.printf("AUX%u%u\r", radio + 1, currentAntenna[radio]);
            } else if (valStr[0] != '\0') {
                int val = atoi(valStr);
                if (val >= 0 && val <= 6) {
                    selectAntenna(radio, val);
                }
            }
        } else if (isQuery) {
            // Unknown AUX port - echo back
            response.printf("?AUX%s\r", rest);
        }
        return;
    }

    // BAND command
    if ((plen = prefixLen(body, "BAND")) > 0) {
        const char* rest = body + plen;
        if (rest[0] == '1' || rest[0] == '2') {
            uint8_t radio = rest[0] - '1';
            if (isQuery) {
                response.printf("BAND%u%s\r", radio + 1, otrspState.band[radio].c_str());
            } else if (rest[1] != '\0') {
                otrspState.band[radio] = String(rest + 1);
            }
        }
        return;
    }

    // MODE command
    if ((plen = prefixLen(body, "MODE")) > 0) {
        const char* rest = body + plen;
        if (rest[0] == '1' || rest[0] == '2') {
            uint8_t radio = rest[0] - '1';
            if (isQuery) {
                response.printf("MODE%u%c\r", radio + 1, otrspState.mode[radio]);
            } else if (rest[1] != '\0') {
                otrspState.mode[radio] = rest[1];
            }
        }
        return;
    }

    // NAME command
    if ((plen = prefixLen(body, "NAME")) > 0) {
        if (isQuery) {
            response.printf("NAME6x2 Antenna Switch SQ9NJE\r");
        }
        // Set NAME is ignored per spec
        return;
    }

    // FW command
    if ((plen = prefixLen(body, "FW")) > 0) {
        if (isQuery) {
            response.print("FW1.0.0\r");
        }
        // Set FW is ignored per spec
        return;
    }

    // Unknown command
    if (isQuery) {
        // Echo back unknown query
        response.printf("?%s\r", body);
    }
    // Unknown non-query commands are silently ignored per spec
}

void initializeOTRSP() {
    if (otrspEnabled) {
        otrspServer.begin();
        if (otrspServer) {
            Serial.printf("OTRSP TCP server listening on port %d\n", OTRSP_TCP_PORT);
        } else {
            Serial.printf("OTRSP TCP server FAILED to start on port %d\n", OTRSP_TCP_PORT);
        }
    } else {
        Serial.println("OTRSP TCP server disabled");
    }
}

void handleOTRSPLoop() {
    if (!otrspEnabled) return;

    // Accept new client
    if (otrspServer.hasClient()) {
        WiFiClient newClient = otrspServer.available();
        if (newClient) {
            if (otrspClient && otrspClient.connected()) {
                // Reject if already have a client
                newClient.stop();
            } else {
                otrspClient = newClient;
                otrspState.clientConnected = true;
                tcpBufLen = 0;
                Serial.println("OTRSP client connected");
            }
        }
    }

    // Handle connected client
    if (otrspClient && otrspClient.connected()) {
        while (otrspClient.available()) {
            char c = otrspClient.read();
            if (c == '\r') {
                tcpBuffer[tcpBufLen] = '\0';
                parseOTRSPCommand(tcpBuffer, otrspClient);
                tcpBufLen = 0;
            } else if (c != '\n' && tcpBufLen < OTRSP_BUF_SIZE - 1) {
                tcpBuffer[tcpBufLen++] = c;
            }
        }
    } else if (otrspState.clientConnected) {
        otrspState.clientConnected = false;
        Serial.println("OTRSP client disconnected");
    }

    // Handle OTRSP on UART2 if enabled
    if (otrspSerialEnabled) {
        handleOTRSPSerialInput(Serial2);
    }
}

void handleOTRSPSerialInput(Stream& serial) {
    while (serial.available()) {
        char c = serial.read();
        if (c == '\r') {
            serialBuffer[serialBufLen] = '\0';
            parseOTRSPCommand(serialBuffer, serial);
            serialBufLen = 0;
        } else if (c != '\n' && serialBufLen < OTRSP_BUF_SIZE - 1) {
            serialBuffer[serialBufLen++] = c;
        }
    }
}
