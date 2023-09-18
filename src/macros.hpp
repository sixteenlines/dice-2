#ifndef _ARDUINO_H
#define _ARDUINO_H
#include <Arduino.h>
#endif

#ifndef _MACROS_HPP
#define _MACROS_HPP

/* Pins */
#define STATUSLED_PIN D4
#define RETENTION_PIN D1
#define LEDS_PIN D8
#define BTN0_PIN D7
#define BTN1_PIN D6
#define BTN2_PIN D5
#define NUM_LEDS 25

/* file path offset macros */
#define _SSID 0
#define _PASS 1
#define _IPAD 2
#define _GATE 3
#define _SUBN 4
#define _DEVICE_TO 5
#define _LED_TO 6
#define _ROLL_DELAY 7

/* LED-pattern macros */
#define BIGDOT 7
#define LOADING 8

/* Serial format macros */
const String INDENT = "         ";

#endif