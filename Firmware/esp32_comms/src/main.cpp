#include <Arduino.h>
#include <WiFi.h>
#include <WiFiManager.h>
#include <ESPAsyncWebServer.h>
#include <WebSocketsServer.h>
#include <ArduinoJson.h>
#include <SPIFFS.h>
#include <ESPmDNS.h>

#define BUF_SIZE   32

// Status LED pin
#define STATUS_LED  32
// Built-in LED pin (already defined in pins_arduino.h)

// UART2 pins (can be configured)
#define RXD2 16
#define TXD2 17

uint8_t currentAntenna[2] = {0, 0}; // 0 means disconnected
// ESP32 GPIO pin mapping - using suitable output pins
const uint8_t relay[2][6] = {{25, 26, 27, 14, 12, 13}, {5, 5, 5, 4, 4, 4}};

// Web server and WebSocket
AsyncWebServer server(80);
WebSocketsServer webSocket = WebSocketsServer(81);

// Antenna names/descriptions
String antennaNames[6] = {"Antenna 1", "Antenna 2", "Antenna 3", "Antenna 4", "Antenna 5", "Antenna 6"};

// mDNS hostname
String mdnsHostname = "antenna";

void blink(uint8_t n) {
  for(uint8_t i = 0; i < n; i++) {
    digitalWrite(STATUS_LED, 1);
    delay(50);
    digitalWrite(STATUS_LED, 0);
    delay(50);  
  }
}

void sendWebSocketUpdate() {
  DynamicJsonDocument doc(200);
  doc["type"] = "state";
  doc["radio1"] = currentAntenna[0];
  doc["radio2"] = currentAntenna[1];
  
  String message;
  serializeJson(doc, message);
  webSocket.broadcastTXT(message);
}

void sendAntennaNameUpdate() {
  DynamicJsonDocument doc(1024);
  doc["type"] = "antennaNames";
  JsonArray array = doc.createNestedArray("names");
  for(int i = 0; i < 6; i++) {
    array.add(antennaNames[i]);
  }
  
  String message;
  serializeJson(doc, message);
  webSocket.broadcastTXT(message);
}

uint8_t selectAntenna(uint8_t radio, uint8_t antenna) {
  if(radio > 1 || antenna > 6) {
    blink(3);
    return 1;
  }
  
  // Check if antenna is already selected by the other radio (unless disconnecting)
  if(antenna > 0) {
    if(radio == 0) {
      if(currentAntenna[1] == antenna) {
        blink(3);
        return 2;
      }    
    }
    else {
      if(currentAntenna[0] == antenna) {
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

void parseCommand(char* commandLine, Stream& responseStream) {
  char* cmd = strsep(&commandLine, " ");
  
  if(strcmp(cmd, "blink") == 0) {
    int num = atoi(strsep(&commandLine, " "));
    blink(num);
  } 
  else if(strcmp(cmd, "set") == 0) {
    int r = atoi(strsep(&commandLine, " "));
    int a = atoi(strsep(&commandLine, " "));
    int result = selectAntenna(r-1, a);
    if(result == 0)
      responseStream.println("+OK");
    else if(result == 1)
      responseStream.println("!ERR");
    else if(result == 2)
      responseStream.println("!BUSY");
  }
  else if(strcmp(cmd, "get") == 0) {
    int r = atoi(strsep(&commandLine, " "));
    responseStream.println(currentAntenna[r-1]);
  }
  else if(strcmp(cmd, "?") == 0) {
    responseStream.println("6x2 Antenna Switch SQ9NJE");
  }
  else if(strcmp(cmd, "test") == 0) {
    for(uint8_t r = 0; r < 2; r++)
      for(int8_t a = 6; a >= 0; a--) {
        selectAntenna(r, a);
        delay(100);
      }
  }
}

void handleSerialInput(Stream& serial, Stream& responseStream) {
  while(serial.available()) {
    static char buffer[BUF_SIZE];
    static uint8_t len = 0;

    char data = serial.read();
    if(data == '\r' || data == '\n') {
      buffer[len] = '\0';
      parseCommand(buffer, responseStream);
      len = 0;
    }
    else if(len < BUF_SIZE-1)
      buffer[len++] = tolower(data);
  }
}

void loadAntennaNames() {
  if(SPIFFS.exists("/antenna_names.json")) {
    File file = SPIFFS.open("/antenna_names.json", "r");
    if(file) {
      DynamicJsonDocument doc(1024);
      deserializeJson(doc, file);
      file.close();
      
      for(int i = 0; i < 6; i++) {
        if(doc.containsKey(String(i))) {
          antennaNames[i] = doc[String(i)].as<String>();
        }
      }
    }
  }
}

void saveAntennaNames() {
  DynamicJsonDocument doc(1024);
  for(int i = 0; i < 6; i++) {
    doc[String(i)] = antennaNames[i];
  }
  
  File file = SPIFFS.open("/antenna_names.json", "w");
  if(file) {
    serializeJson(doc, file);
    file.close();
  }
}

void loadSettings() {
  if(SPIFFS.exists("/settings.json")) {
    File file = SPIFFS.open("/settings.json", "r");
    if(file) {
      DynamicJsonDocument doc(512);
      deserializeJson(doc, file);
      file.close();
      
      if(doc.containsKey("mdnsHostname")) {
        mdnsHostname = doc["mdnsHostname"].as<String>();
      }
    }
  }
}

void saveSettings() {
  DynamicJsonDocument doc(512);
  doc["mdnsHostname"] = mdnsHostname;
  
  File file = SPIFFS.open("/settings.json", "w");
  if(file) {
    serializeJson(doc, file);
    file.close();
  }
}

void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length) {
  switch(type) {
    case WStype_DISCONNECTED:
      Serial.printf("[%u] Disconnected!\n", num);
      break;
      
    case WStype_CONNECTED:
      Serial.printf("[%u] Connected from %s\n", num, webSocket.remoteIP(num).toString().c_str());
      sendWebSocketUpdate(); // Send current state to new client
      sendAntennaNameUpdate(); // Send antenna names to new client
      break;
      
    case WStype_TEXT:
      {
        String message = String((char*)payload);
        DynamicJsonDocument doc(200);
        deserializeJson(doc, message);
        
        if(doc["type"] == "select") {
          uint8_t radio = doc["radio"];
          uint8_t antenna = doc["antenna"];
          selectAntenna(radio, antenna);
        }
      }
      break;
      
    default:
      break;
  }
}

void setup() {
  Serial.begin(115200);
  Serial2.begin(9600, SERIAL_8N1, RXD2, TXD2);
  
  Serial.println("Starting 6x2 Antenna Switch SQ9NJE");

  // Initialize SPIFFS
  if(!SPIFFS.begin(true)) {
    Serial.println("SPIFFS Mount Failed");
    return;
  }

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

  // Load settings and antenna names
  loadSettings();
  loadAntennaNames();

  // WiFiManager with custom hostname parameter
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

  // Initialize mDNS
  if (MDNS.begin(mdnsHostname.c_str())) {
    Serial.println("mDNS responder started");
    Serial.print("You can now connect to http://");
    Serial.print(mdnsHostname);
    Serial.println(".local");
    
    // Add service to mDNS
    MDNS.addService("http", "tcp", 80);
    MDNS.addService("ws", "tcp", 81);
  } else {
    Serial.println("Error setting up mDNS responder!");
  }

  // WebSocket setup
  webSocket.begin();
  webSocket.onEvent(webSocketEvent);

  // Web server routes
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/index.html", "text/html");
  });

  server.on("/settings", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/settings.html", "text/html");
  });

  server.on("/style.css", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/style.css", "text/css");
  });

  server.on("/script.js", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/script.js", "text/javascript");
  });

  server.on("/api/antennas", HTTP_GET, [](AsyncWebServerRequest *request){
    DynamicJsonDocument doc(1024);
    JsonArray array = doc.to<JsonArray>();
    for(int i = 0; i < 6; i++) {
      array.add(antennaNames[i]);
    }
    String response;
    serializeJson(doc, response);
    request->send(200, "application/json", response);
  });

  server.on("/api/antennas", HTTP_POST, [](AsyncWebServerRequest *request){}, NULL, 
    [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total){
      DynamicJsonDocument doc(1024);
      deserializeJson(doc, (char*)data);
      
      for(int i = 0; i < 6; i++) {
        if(doc.containsKey(String(i))) {
          antennaNames[i] = doc[String(i)].as<String>();
        }
      }
      
      saveAntennaNames();
      sendAntennaNameUpdate(); // Broadcast updated names via WebSocket
      request->send(200, "text/plain", "OK");
    });

  // Individual antenna name management
  server.on("^\\/api\\/antenna\\/(\\d+)$", HTTP_GET, [](AsyncWebServerRequest *request){
    String antennaStr = request->pathArg(0);
    int antennaIndex = antennaStr.toInt();
    
    if(antennaIndex >= 0 && antennaIndex < 6) {
      DynamicJsonDocument doc(256);
      doc["index"] = antennaIndex;
      doc["name"] = antennaNames[antennaIndex];
      
      String response;
      serializeJson(doc, response);
      request->send(200, "application/json", response);
    } else {
      request->send(400, "text/plain", "Invalid antenna index");
    }
  });

  server.on("^\\/api\\/antenna\\/(\\d+)$", HTTP_PUT, [](AsyncWebServerRequest *request){}, NULL,
    [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total){
      String antennaStr = request->pathArg(0);
      int antennaIndex = antennaStr.toInt();
      
      if(antennaIndex >= 0 && antennaIndex < 6) {
        DynamicJsonDocument doc(256);
        deserializeJson(doc, (char*)data);
        
        if(doc.containsKey("name")) {
          antennaNames[antennaIndex] = doc["name"].as<String>();
          saveAntennaNames();
          sendAntennaNameUpdate(); // Broadcast updated names via WebSocket
          request->send(200, "text/plain", "OK");
        } else {
          request->send(400, "text/plain", "Missing 'name' field");
        }
      } else {
        request->send(400, "text/plain", "Invalid antenna index");
      }
    });

  server.on("/api/state", HTTP_GET, [](AsyncWebServerRequest *request){
    DynamicJsonDocument doc(200);
    doc["radio1"] = currentAntenna[0];
    doc["radio2"] = currentAntenna[1];
    String response;
    serializeJson(doc, response);
    request->send(200, "application/json", response);
  });

  // mDNS hostname management
  server.on("/api/hostname", HTTP_GET, [](AsyncWebServerRequest *request){
    DynamicJsonDocument doc(256);
    doc["hostname"] = mdnsHostname;
    String response;
    serializeJson(doc, response);
    request->send(200, "application/json", response);
  });

  server.on("/api/hostname", HTTP_POST, [](AsyncWebServerRequest *request){}, NULL, 
    [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total){
      DynamicJsonDocument doc(256);
      deserializeJson(doc, (char*)data);
      
      if(doc.containsKey("hostname")) {
        String newHostname = doc["hostname"].as<String>();
        
        // Validate hostname (basic validation)
        if(newHostname.length() > 0 && newHostname.length() <= 63) {
          // Remove invalid characters and convert to lowercase
          String validHostname = "";
          for(int i = 0; i < newHostname.length(); i++) {
            char c = newHostname[i];
            if((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '-') {
              validHostname += tolower(c);
            }
          }
          
          if(validHostname.length() > 0) {
            mdnsHostname = validHostname;
            saveSettings();
            request->send(200, "text/plain", "OK - Restart required for changes to take effect");
          } else {
            request->send(400, "text/plain", "Invalid hostname format");
          }
        } else {
          request->send(400, "text/plain", "Hostname length must be 1-63 characters");
        }
      } else {
        request->send(400, "text/plain", "Missing 'hostname' field");
      }
    });

  server.begin();
  Serial.println("HTTP server started");

  blink(3);
  Serial.println("System ready");
}

void loop() {
  webSocket.loop();
  
  // Handle UART0 (Serial) commands - debug output
  handleSerialInput(Serial, Serial);
  
  // Handle UART2 commands - debug output to Serial
  handleSerialInput(Serial2, Serial);
}
