#include "macros.hpp"
#include <ESP8266WiFi.h>
#include <ESPAsyncWebServer.h>
#include <ESPAsyncTCP.h>
#include <DNSServer.h>
#include <Arduino.h>

/* Endpoint params main webpage */
const char *PARAM_R = "r";
const char *PARAM_G = "g";
const char *PARAM_B = "b";
const char *PARAM_LED = "led";
const char *PARAM_RESULT = "result";
const char *PARAM_LED_TIMEOUT = "toLed";
const char *PARAM_DEVICE_TIMEOUT = "toDevice";

/* Endpoint params wifi manager */
const char *PARAM_INPUT_0 = "ssid";
const char *PARAM_INPUT_1 = "pass";
const char *PARAM_INPUT_2 = "ip";
const char *PARAM_INPUT_3 = "gateway";
const char *PARAM_INPUT_4 = "subnet";
const String MANAGER = "http://8.8.8.8";

/* Access Point settings */
const String AP_SSID = "MAGIC-DICE-SETUP";
const String AP_PW = "eisdiele";

/* dice vars */
uint8_t dieRollResult;

bool rollRequested;
bool sleepRequested;
bool managerRequested;

uint8_t diceRed;
uint8_t diceGreen;
uint8_t diceBlue;

unsigned long lastActionTime;

bool initWifi();
bool clientSetup();
void managerSetup();
void hostManager();
void hostIndex();
void dnsNext();
bool handleFileRequest(AsyncWebServerRequest *request, String path);
