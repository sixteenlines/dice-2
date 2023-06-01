#include <vector>
#include <Adafruit_NeoPixel.h>
#include <ESP8266WiFi.h>
#include <ESPAsyncWebServer.h>
#include <ESPAsyncTCP.h>
#include <LittleFS.h>
#include <DNSServer.h>
#include <Arduino.h>

/* Pins */
#define STATUSLED_PIN D4
#define RETENTION_PIN D1
#define LEDS_PIN D2
#define BTN0_PIN D6
#define BTN1_PIN D7
#define BTN2_PIN D5
#define NUMPIXELS 25

/* credential offset macros */
#define _SSID 0
#define _PASS 1
#define _IPAD 2
#define _GATE 3
#define _SUBN 4

/* Timer constants and dns port */
const unsigned long DEEP_SLEEP = 180000;
const unsigned long LIGHTS_OFF = 20000;

/* Endpoint params index */
const char *PARAM_R = "r";
const char *PARAM_G = "g";
const char *PARAM_B = "b";
const char *PARAM_LED = "led";
const char *PARAM_RESULT = "result";

/* Endpoint params manager */
const char *PARAM_INPUT_0 = "ssid";
const char *PARAM_INPUT_1 = "pass";
const char *PARAM_INPUT_2 = "ip";
const char *PARAM_INPUT_3 = "gateway";
const char *PARAM_INPUT_4 = "subnet";

/* File paths to storage */
const std::vector<String> paths = {
    "/ssid.txt",
    "/password.txt",
    "/ip.txt",
    "/gateway.txt",
    "/subnet.txt"};

/* LED-pattern vector */
const std::vector<std::vector<int>> dicePatterns = {
    {},                    // symbol 0
    {12},                  // symbol 1
    {4, 20},               // symbol 2
    {4, 12, 20},           // symbol 3
    {0, 4, 20, 24},        // symbol 4
    {0, 4, 12, 20, 24},    // symbol 5
    {0, 4, 10, 14, 20, 24} // symbol 6
};

/* function declarations */
void prerolldice(void);
void marius(void);
bool initWifi(void);
bool clientSetup(void);
void managerSetup(void);
void initIO(void);
void hostManager(void);
void hostIndex(void);
bool loadCredentials(void);
void printPattern(uint8_t pattern);
void setLED(uint8_t num);
void clearLED(uint8_t num);
void initFS(void);
void writeFile(const String path, const char *message);
String readFile(const String path);