#ifndef _ARDUINO_H
#define _ARDUINO_H
#include <Arduino.h>
#endif

#ifndef _VECTOR_H
#define _VECTOR_H
#include <vector>
#endif

#ifndef _FS_HPP
#define _FS_HPP

#include <LittleFS.h>
#include <Arduino_JSON.h>

/* file paths to storage */
const std::vector<String> paths = {
    "/creds/ssid.txt",
    "/creds/password.txt",
    "/creds/ip.txt",
    "/creds/gateway.txt",
    "/creds/subnet.txt",
    "/settings/devicetimeout.txt",
    "/settings/ledtimeout.txt",
    "/settings/rolldelay.txt"};

void initFS();
void initSettings();
String getSettings();
void writeFile(const String path, const char *message);
String readFile(const String path);
bool loadCredentials();

#endif