#ifndef _WS_HPP
#define _WS_HPP

#include "macros.hpp"
#include <ESP8266WiFi.h>
#include <ESPAsyncWebServer.h>
#include <ESPAsyncTCP.h>
#include <DNSServer.h>
#include <Arduino.h>

bool initWifi();
bool clientSetup();
void managerSetup();
void hostManager();
void hostIndex();
void dnsNext();
bool handleFileRequest(AsyncWebServerRequest *request, String path);

#endif