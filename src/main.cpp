#include <webserver.hpp>
#include <filesystem.hpp>
#include <ledmatrix.hpp>
#include <macros.hpp>
#include <Arduino.h>

/* external vars */
extern uint8_t dieRollResult;

extern bool rollRequested;
extern bool sleepRequested;
extern bool managerRequested;

extern uint8_t diceRed;
extern uint8_t diceGreen;
extern uint8_t diceBlue;

extern unsigned long lastActionTime;

extern unsigned long deep_sleep;
extern unsigned long led_sleep;

/* function declarations */
void sleep();
void initIO();
void rollDice();
void prerollDice();

/* timer var */
unsigned long currentTime = 0;
/* preroll var */
uint8_t lastPreroll = 0;

/*###################################### SETUP ##############################*/
void setup()
{
    initIO();
    initFS();
    initSettings();
    initLeds();

    if (initWifi())
    {
        Serial.println("[\e[0;32m  OK  \e[0;37m] Initializing WiFi");
    }
    else
    {
        if (sleepRequested)
        {
            Serial.println("[\e[0;31mFAILED\e[0;37m] Initializing WiFi");
            Serial.println(INDENT + "Going to Sleep");
        }
        else
        {
            Serial.println("[\e[0;32m  OK  \e[0;37m] Offline Mode");
        }
    }
}

/*###################################### MAIN ###############################*/
void loop()
{
    if (managerRequested)
    {
        dnsNext();
    }
    else
    {
        if (sleepRequested)
        {
            sleep();
        }
        currentTime = millis();

        if (!(digitalRead(BTN0_PIN)))
        {
            dieRollResult = random(6) + 1;
            rollRequested = true;
        }

        if (rollRequested)
        {
            rollDice();
        }
        if (currentTime - lastActionTime >= deep_sleep)
        {
            sleep();
        }
        if (currentTime - lastActionTime >= led_sleep)
        {
            hideLEDS();
        }
    }
}

void initIO()
{
    Serial.begin(115200);
    Serial.println();
    pinMode(BTN0_PIN, INPUT_PULLUP); // used for rolling manually
    pinMode(BTN1_PIN, INPUT_PULLUP); // hold on boot for wifi manager
    pinMode(BTN2_PIN, INPUT_PULLUP); // free
    pinMode(RETENTION_PIN, OUTPUT);
    pinMode(STATUSLED_PIN, OUTPUT); // can be used to control onboard led
    digitalWrite(RETENTION_PIN, HIGH);
    Serial.println("[\e[0;32m  OK  \e[0;37m] Initializing system I/O");
}

// prints a random die roll to the matrix
void prerollDice()
{
    uint8_t preroll = random(6) + 1;
    while (preroll == lastPreroll)
    {
        preroll = random(6) + 1;
    }
    lastPreroll = preroll;
    printPattern(preroll, diceRed, diceGreen, diceBlue);
}

void rollDice()
{
    // rolling a few times to simulate a real die
    static unsigned short prerollDisplayDuration = 50;

    if (prerollDisplayDuration == 300)
    {
        if (currentTime - lastActionTime >= 400)
        {
            printPattern(dieRollResult, diceRed, diceGreen, diceBlue);
            rollRequested = false;
            prerollDisplayDuration = 50;
        }
    }
    else if (currentTime - lastActionTime >= prerollDisplayDuration)
    {
        prerollDice();
        lastActionTime = currentTime;
        prerollDisplayDuration += 50;
    }
}

// pulls self-retention pin low, device turns off
void sleep()
{
    digitalWrite(RETENTION_PIN, LOW);
}