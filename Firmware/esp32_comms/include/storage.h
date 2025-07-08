#ifndef STORAGE_H
#define STORAGE_H

#include <Arduino.h>

/**
 * @brief Initialize SPIFFS storage
 * @return true if successful, false otherwise
 */
bool initializeStorage();

/**
 * @brief Load antenna names from storage
 */
void loadAntennaNames();

/**
 * @brief Save antenna names to storage
 */
void saveAntennaNames();

/**
 * @brief Load settings from storage
 */
void loadSettings();

/**
 * @brief Save settings to storage
 */
void saveSettings();

/**
 * @brief Validate and sanitize hostname
 * @param input Input hostname string
 * @return Valid hostname or empty string if invalid
 */
String validateHostname(const String& input);

#endif
