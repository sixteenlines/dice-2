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
#define NUM_LEDS 25

/* credential offset macros */
#define _SSID 0
#define _PASS 1
#define _IPAD 2
#define _GATE 3
#define _SUBN 4

/* LED-pattern macros */
#define BIGDOT 7
#define LOADING 8

/* Timer constants */
const unsigned long DEEP_SLEEP = 120000;
const unsigned long LIGHTS_OFF = 20000;

/* Endpoint params index */
const char *PARAM_R = "r";
const char *PARAM_G = "g";
const char *PARAM_B = "b";
const char *PARAM_LED = "led";
const char *PARAM_POWER = "power";
const char *PARAM_RESULT = "result";

/* Endpoint params manager */
const char *PARAM_INPUT_0 = "ssid";
const char *PARAM_INPUT_1 = "pass";
const char *PARAM_INPUT_2 = "ip";
const char *PARAM_INPUT_3 = "gateway";
const char *PARAM_INPUT_4 = "subnet";
const String MANAGER = "http://8.8.8.8";

/* File paths to storage */
const std::vector<String> paths = {
    "/ssid.txt",
    "/password.txt",
    "/ip.txt",
    "/gateway.txt",
    "/subnet.txt"};

/* LED-pattern vector */
const std::vector<std::vector<int>> patterns = {
    {},                                // symbol 0
    {12},                              // symbol 1
    {4, 20},                           // symbol 2
    {4, 12, 20},                       // symbol 3
    {0, 4, 20, 24},                    // symbol 4
    {0, 4, 12, 20, 24},                // symbol 5
    {0, 4, 10, 14, 20, 24},            // symbol 6
    {6, 7, 8, 11, 12, 13, 16, 17, 18}, // BIGDOT 7
    {0, 1, 2, 3, 4, 9, 14,
     19, 24, 23, 22, 21,
     20, 15, 10, 5} // LOADING 8
};

const uint8_t loading[] = {0, 1, 2, 3, 4, 9, 14,
                           19, 24, 23, 22, 21,
                           20, 15, 10, 5};

/* led class */
class led
{
public:
    uint8_t r, g, b;
    bool power;

    led() : r(0), g(0), b(0), power(false) {}
};

/* function declarations */
void prerolldice(void);
void marius(void);
bool initWifi(void);
bool clientSetup(void);
void managerSetup(void);
void initIO(void);
void initLeds(void);
void hostManager(void);
void hostIndex(void);
bool loadCredentials(void);
void printPattern(uint8_t pattern, uint8_t r, uint8_t g, uint8_t b);
void printPattern(uint8_t pattern);
void setLED(uint8_t num, uint8_t r, uint8_t g, uint8_t b, bool power);
void setLED(uint8_t num, bool power);
void hideLEDS();
void updateLEDS();
void initFS(void);
bool handleFileRequest(AsyncWebServerRequest *request, String path);
void writeFile(const String path, const char *message);
String readFile(const String path);