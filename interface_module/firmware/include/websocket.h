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
 * @brief Send OTA status update via WebSocket
 * @param status Status string (starting, progress, complete, error)
 * @param message Additional message
 * @param progress Progress percentage (0-100)
 */
void sendOTAStatus(const String& status, const String& message, uint8_t progress);

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
