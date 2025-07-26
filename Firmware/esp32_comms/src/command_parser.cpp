#include "command_parser.h"
#include "globals.h"
#include "antenna_hardware.h"

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
