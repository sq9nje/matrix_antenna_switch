#ifndef WEBSOCKET_H
#define WEBSOCKET_H

#include <WebSocketsServer.h>
#include <ArduinoJson.h>

/**
 * @brief Send WebSocket update with current antenna state
 */
void sendWebSocketUpdate();

/**
 * @brief Send antenna names update via WebSocket
 */
void sendAntennaNameUpdate();

/**
 * @brief Handle WebSocket events
 * @param num Client number
 * @param type Event type
 * @param payload Event payload
 * @param length Payload length
 */
void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length);

/**
 * @brief Initialize WebSocket server
 */
void initializeWebSocket();

#endif
