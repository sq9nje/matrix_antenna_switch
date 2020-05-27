#include <Arduino.h>
#include <avr/wdt.h>

#define BUF_SIZE   32

uint8_t currentAntenna[2];
const uint8_t relay[2][6] = {{2, 3, 4, 5, 6, 7}, {A0, A1, A2, A3, A4, A5}};

void blink(uint8_t n) {
  for( uint8_t i = 0; i < n; i++) {
    digitalWrite(13, 1);
    delay(50);
    digitalWrite(13, 0);
    delay(50);  
  }
}

uint8_t selectAntenna(uint8_t radio, uint8_t antenna) {
  if(radio > 1 || antenna > 6) {
    blink(3);
    return 1;
  }
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
   
  if(currentAntenna[radio] > 0)
    digitalWrite(relay[radio][currentAntenna[radio]-1], 0);
  if(antenna > 0) 
    digitalWrite(relay[radio][antenna-1], 1);
  currentAntenna[radio] = antenna;
  
  blink(1);
  return 0;
}

void parseCommand(char* commandLine) {

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
      Serial.println("+OK");
    else if(result == 1)
      Serial.println("!ERR");
    else if(result == 2)
      Serial.println("!BUSY");
  }
  else if(strcmp(cmd, "get") == 0) {
    int r = atoi(strsep(&commandLine, " "));
    Serial.println(currentAntenna[r-1]);
  }
  else if(strcmp(cmd, "?") == 0) {
    Serial.println("6x2 Antenna Switch SQ9NJE");
  }
}

void setup() {
  wdt_enable(WDTO_1S);
  Serial.begin(9600);

  pinMode(A0, OUTPUT);
  pinMode(A1, OUTPUT);
  pinMode(A2, OUTPUT);
  pinMode(A3, OUTPUT);
  pinMode(A4, OUTPUT);
  pinMode(A5, OUTPUT);
  pinMode(2, OUTPUT);
  pinMode(3, OUTPUT);
  pinMode(4, OUTPUT);
  pinMode(5, OUTPUT);
  pinMode(6, OUTPUT);
  pinMode(7, OUTPUT);
  pinMode(13, OUTPUT);

  blink(5);
}

void loop() {
  while(1) {
    wdt_reset();

    while(Serial.available()) {
      static char buffer[BUF_SIZE];
      static uint8_t len = 0;

      char data = Serial.read();
      if(data == '\r' || data == '\n') {
        buffer[len] = '\0';
        parseCommand(buffer);
        len = 0;
      }
      else if(len < BUF_SIZE-1)
        buffer[len++] = tolower(data);
    }
  }
}

