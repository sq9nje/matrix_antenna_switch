#include "antenna_hardware.h"
#include "globals.h"
#include "websocket.h"

void initializeHardware() {
  // Initialize all relay control pins as outputs
  for(uint8_t radio = 0; radio < 2; radio++) {
    for(uint8_t antenna = 0; antenna < 6; antenna++) {
      pinMode(relay[radio][antenna], OUTPUT);
      digitalWrite(relay[radio][antenna], LOW);
    }
  }
  
  // Initialize LED pins
  pinMode(STATUS_LED, OUTPUT);
  pinMode(BUILTIN_LED, OUTPUT);
}

void blink(uint8_t n) {
  for(uint8_t i = 0; i < n; i++) {
    digitalWrite(STATUS_LED, 1);
    delay(50);
    digitalWrite(STATUS_LED, 0);
    delay(50);  
  }
}

uint8_t selectAntenna(uint8_t radio, uint8_t antenna) {
  if(radio > 1 || antenna > 6) {
    blink(3);
    return 1;
  }
  
  // Single radio mode - always disconnect radio 2
  if(singleRadioMode && radio == 1) {
    if(currentAntenna[1] > 0) {
      digitalWrite(relay[1][currentAntenna[1]-1], 0);
      currentAntenna[1] = 0;
    }
    sendWebSocketUpdate();
    blink(1);
    return 0;
  }
  
  // Check if antenna is already selected by the other radio (unless disconnecting)
  if(antenna > 0) {
    uint8_t otherRadio = (radio == 0) ? 1 : 0;
    
    if(currentAntenna[otherRadio] == antenna) {
      // If antenna swapping is enabled, perform the swap
      if(antennaSwappingEnabled) {
        uint8_t previousAntenna = currentAntenna[radio];
        
        // Step 1: Disconnect the other radio
        digitalWrite(relay[otherRadio][antenna-1], 0);
        
        // Step 2: Connect the first radio to the desired antenna
        if(currentAntenna[radio] > 0) {
          digitalWrite(relay[radio][currentAntenna[radio]-1], 0);
        }
        digitalWrite(relay[radio][antenna-1], 1);
        currentAntenna[radio] = antenna;
        
        // Step 3: Connect the other radio to the antenna that was used by the first radio
        if(previousAntenna > 0 && !singleRadioMode) {
          digitalWrite(relay[otherRadio][previousAntenna-1], 1);
          currentAntenna[otherRadio] = previousAntenna;
        } else {
          currentAntenna[otherRadio] = 0;
        }
        
        sendWebSocketUpdate();
        blink(1);
        return 0;
      } else {
        // Antenna swapping disabled - return busy error
        blink(3);
        return 2;
      }
    }
  }
   
  // Turn off current antenna relay
  if(currentAntenna[radio] > 0)
    digitalWrite(relay[radio][currentAntenna[radio]-1], 0);
  
  // Turn on new antenna relay (if not disconnecting)
  if(antenna > 0) 
    digitalWrite(relay[radio][antenna-1], 1);
  
  currentAntenna[radio] = antenna;
  
  // Send WebSocket update
  sendWebSocketUpdate();
  
  blink(1);
  return 0;
}
