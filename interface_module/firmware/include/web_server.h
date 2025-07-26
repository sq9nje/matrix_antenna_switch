#ifndef WEB_SERVER_H
#define WEB_SERVER_H

#include <ESPAsyncWebServer.h>

/**
 * @brief Initialize and configure the web server
 */
void initializeWebServer();

/**
 * @brief Initialize mDNS service
 */
void initializeMDNS();

#endif
