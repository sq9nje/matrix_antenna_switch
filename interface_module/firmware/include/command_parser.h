#ifndef COMMAND_PARSER_H
#define COMMAND_PARSER_H

#include <Arduino.h>

/**
 * @brief Parse and execute a command
 * @param commandLine Command line string
 * @param responseStream Stream to send response to
 */
void parseCommand(char* commandLine, Stream& responseStream);

/**
 * @brief Handle serial input from a stream
 * @param serial Input stream
 * @param responseStream Output stream for responses
 */
void handleSerialInput(Stream& serial, Stream& responseStream);

#endif
