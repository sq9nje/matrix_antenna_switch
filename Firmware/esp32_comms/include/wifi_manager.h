#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <Arduino.h>

/**
 * @brief Initialize WiFi connection with WiFiManager
 */
void initializeWiFi();

/**
 * @brief Reset WiFi settings and restart device
 */
void resetNetworkSettings();

#endif
