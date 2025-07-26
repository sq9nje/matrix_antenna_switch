#ifndef ANTENNA_HARDWARE_H
#define ANTENNA_HARDWARE_H

#include <Arduino.h>

/**
 * @brief Initialize all hardware pins
 */
void initializeHardware();

/**
 * @brief Blink the status LED
 * @param n Number of blinks
 */
void blink(uint8_t n);

/**
 * @brief Select antenna for a specific radio
 * @param radio Radio number (0 or 1)
 * @param antenna Antenna number (0-6, 0 means disconnect)
 * @return 0 on success, 1 on parameter error, 2 on antenna busy
 */
uint8_t selectAntenna(uint8_t radio, uint8_t antenna);

#endif
