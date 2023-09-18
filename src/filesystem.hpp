#include "macros.hpp"
#include <vector>
#include <LittleFS.h>
#include <Arduino_JSON.h>
#include <Arduino.h>

/* file paths to storage */
const std::vector<String> paths = {
    "/creds/ssid.txt",
    "/creds/password.txt",
    "/creds/ip.txt",
    "/creds/gateway.txt",
    "/creds/subnet.txt",
    "/settings/devicetimeout.txt",
    "/settings/ledtimeout.txt"};

/* Credentials in RAM */
String creds[5];

unsigned long deep_sleep;
unsigned long led_sleep;

void initFS();
void initSettings();
String getSettings();
void writeFile(const String path, const char *message);
String readFile(const String path);
bool loadCredentials();