#include <webserver.hpp>
#include <filesystem.hpp>
#include <ledmatrix.hpp>
#include <macros.hpp>
#include <Arduino.h>

/* function declarations */
void sleep();
void initIO();
void prerolldice();

/* timer var */
unsigned long currentTime = 0;

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
        Serial.println("[\e[0;31mFAILED\e[0;37m] Initializing WiFi");
        sleepRequested = true; // optional power down if Wifi fails
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
            // rolling a few times to simulate a real die
            static unsigned short prerollDisplayDuration = 50;
            if (currentTime - lastActionTime >= prerollDisplayDuration)
            {
                prerolldice();
                lastActionTime = currentTime;
                prerollDisplayDuration += 50;
            }
            if (prerollDisplayDuration == 400)
            {
                printPattern(dieRollResult, diceRed, diceGreen, diceBlue);
                rollRequested = false;
                prerollDisplayDuration = 50;
            }
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
void prerolldice()
{
    uint8_t preroll = random(6) + 1;
    printPattern(preroll, diceRed, diceGreen, diceBlue);
}

// pulls self-retention pin low, device turns off
void sleep()
{
    digitalWrite(RETENTION_PIN, LOW);
}