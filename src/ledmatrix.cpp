#include "ledmatrix.hpp"
#include "webserver.hpp"

/* external vars */
extern uint8_t diceRed;
extern uint8_t diceGreen;
extern uint8_t diceBlue;

extern unsigned long lastActionTime;

/* Initializing neopixel and grid*/
Adafruit_NeoPixel pixels =
    Adafruit_NeoPixel(NUM_LEDS, LEDS_PIN, NEO_GRB + NEO_KHZ400);

std::vector<led> ledgrid;

// Creates 25 LED objects for easy manip
void initLeds()
{
    for (int i = 0; i < NUM_LEDS; i++)
    {
        ledgrid.push_back(led());
    }

    pixels.begin();
    showLEDS();
    Serial.println("[\e[0;32m  OK  \e[0;37m] Initializing LED matrix");
}

/*############################ LED MATRIX CONTROL ###########################*/

// Print pattern in specific color
void printPattern(uint8_t pattern, uint8_t r, uint8_t g, uint8_t b)
{
    for (int num = 0; num < 25; num++)
    {
        setLED(num, 0, 0, 0);
    }
    const std::vector<int> &dice = patterns[pattern];
    for (int num : dice)
    {
        setLED(num, r, g, b);
    }
    showLEDS();
}

// Print pattern in global color
void printPattern(uint8_t pattern)
{
    for (int num = 0; num < 25; num++)
    {
        setLED(num, 0, 0, 0);
    }
    const std::vector<int> &dice = patterns[pattern];
    for (int num : dice)
    {
        setLED(num, diceRed, diceGreen, diceBlue);
    }
    showLEDS();
}

// Preload color for specific LED
void setLED(uint8_t num, uint8_t r, uint8_t g, uint8_t b)
{
    ledgrid[num].r = r;
    ledgrid[num].g = g;
    ledgrid[num].b = b;
}

// Turns the matrix off without losing current pattern
void hideLEDS()
{
    for (int num = 0; num < NUM_LEDS; num++)
    {
        pixels.setPixelColor(num, pixels.Color(0, 0, 0));
    }
    pixels.show();
}

// Updates the matrix to reflect preloaded LED colors
void showLEDS()
{
    for (int num = 0; num < NUM_LEDS; num++)
    {
        pixels.setPixelColor(num,
                             pixels.Color(ledgrid[num].r,
                                          ledgrid[num].g,
                                          ledgrid[num].b));
    }
    pixels.show();
}