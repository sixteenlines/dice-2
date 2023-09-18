#ifndef _MATRIX_HPP
#define _MATRIX_HPP

#include "macros.hpp"
#include <vector>
#include <Adafruit_NeoPixel.h>
#include <Arduino.h>

/* led class */
class led
{
public:
    uint8_t r, g, b;
    bool power;

    led() : r(0), g(0), b(0), power(false) {}
};

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
    {0, 1, 2, 3, 4, 9, 14,             // SQUARE 8
     19, 24, 23, 22, 21,
     20, 15, 10, 5}};

void initLeds();
void printPattern(uint8_t pattern, uint8_t r, uint8_t g, uint8_t b);
void printPattern(uint8_t pattern);
void setLED(uint8_t num, uint8_t r, uint8_t g, uint8_t b);
void showLEDS();
void hideLEDS();

#endif