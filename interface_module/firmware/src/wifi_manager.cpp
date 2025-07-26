#include "wifi_manager.h"
#include "globals.h"
#include "storage.h"
#include <WiFi.h>
#include <WiFiManager.h>

void initializeWiFi() {
  WiFiManager wm;
  wm.setConfigPortalTimeout(180); // 3 minutes timeout
  
  // Add custom parameter for mDNS hostname
  WiFiManagerParameter custom_hostname("hostname", "mDNS Hostname", mdnsHostname.c_str(), 63);
  wm.addParameter(&custom_hostname);
  
  if(!wm.autoConnect("AntennaSwitch")) {
    Serial.println("Failed to connect");
    ESP.restart();
  }
  
  // Save custom hostname if it was changed
  if (strcmp(custom_hostname.getValue(), mdnsHostname.c_str()) != 0) {
    mdnsHostname = String(custom_hostname.getValue());
    saveSettings();
    Serial.println("Hostname updated from WiFiManager: " + mdnsHostname);
  }

  Serial.println("WiFi connected!");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}

void resetNetworkSettings() {
  WiFiManager wm;
  wm.resetSettings();
  Serial.println("Network settings reset. Device will reboot...");
  delay(1000);
  ESP.restart();
}
