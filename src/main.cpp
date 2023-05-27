#include <Adafruit_NeoPixel.h>
#include <ESP8266WiFi.h>
#include <ESPAsyncWebServer.h>
#include <ESPAsyncTCP.h>
#include <ESPAsync_WiFiManager.h>
#include <ESP8266WiFiMulti.h>
#include <EEPROM.h>
#include <Arduino.h>
#include <vector>

// Pins
#define PIN 2
#define NUMPIXELS 25
Adafruit_NeoPixel pixels = Adafruit_NeoPixel(NUMPIXELS, PIN, NEO_RGB + NEO_KHZ400);

/* Endpoint params */
const char *PARAM_R = "r";
const char *PARAM_G = "g";
const char *PARAM_B = "b";
const char *PARAM_LED = "led";
const char *PARAM_RESULT = "result";

/* LED-Out */
const std::vector<std::vector<int>> dicePatterns = {
    {},                 // symbol 0
    {12},               // symbol 1
    {4, 20},            // symbol 2
    {4, 12, 20},        // symbol 3
    {0, 4, 20, 24},     // symbol 4
    {0, 4, 12, 20, 24}, // symbol 5
    {0, 4, 14, 20, 24}  // symbol 6
};

/* Network AP */
String ssid = "";
String pw = "";
const char *ssidAP = "MAGIC-DICE";
const char *pwAP = "noethmandel";
AsyncWebServer webServer(80);
AsyncDNSServer dnsServer;
ESP8266WiFiMulti wifiMulti;

/* Colors */
uint8_t r = 255;
uint8_t g = 255;
uint8_t b = 255;

/* Control */
uint8_t roll = 0;
unsigned long startMillis = 0;
unsigned long currentMillis = 0;
unsigned short period = 50;
const unsigned long DEEP_SLEEP = 180000;
const unsigned long LIGHTS_OFF = 20000;
bool btn = false;
bool d1 = true;
bool sleep = false;

// web pages
const char index_html[] PROGMEM = {
    R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <title>Roll Dice Webpage</title>
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <style>
        body {
            color: black;
            font-family: Arial, sans-serif;
            margin: 0;
            padding: 0;
        }

        .container {
            display: flex;
            flex-direction: column;
            align-items: center;
            justify-content: center;
            min-height: 100vh;
            padding: 10px;
        }

		.led-grid {
            display: grid;
            grid-template-columns: repeat(5, 20px);
            grid-template-rows: repeat(5, 20px);
            gap: 20px;
            margin-bottom: 20px;
        }

        .led {
            width: 30px;
            height: 30px;
            background: black;
            border: 1px solid black;
            border-radius: 50%;
        }

        .color-picker-container {
            display: flex;
            flex-direction: column;
            align-items: center;
            margin-bottom: 20px;
            width: 100%;
        }

        .color-picker {
            width: 100%;
            max-width: 300px;
            padding: 10px;
            background: white;
            color: black;
            border: 2px solid black;
        }

        .color-picker-heading {
            margin-bottom: 10px;
            -webkit-text-stroke: 1px white;
        }

        .roll-dice-btn {
            padding: 10px 20px;
            background: black;
            color: white;
            border: none;
            cursor: pointer;
            transition: 0.3s;
            width: 100%;
            max-width: 300px;
            margin-bottom: 5px;
        }

        .roll-dice-btn:hover {
            background: white;
            color: black;
            border: 2px solid black;
        }

        .roll-dice-btn:disabled {
            background: gray;
            cursor: not-allowed;
        }

        @media only screen and (max-width: 600px) {
            .color-picker,
            .roll-dice-btn {
                width: 100%;
            }
        }
    </style>
</head>
<body>
    <div class="container">
        <div class="led-grid">
            <div class="led" id="led0"></div><div class="led" id="led1"></div>
            <div class="led" id="led2"></div><div class="led" id="led3"></div>
            <div class="led" id="led4"></div>
            <div class="led" id="led5"></div><div class="led" id="led6"></div>
            <div class="led" id="led7"></div><div class="led" id="led8"></div>
            <div class="led" id="led9"></div>
            <div class="led" id="led10"></div><div class="led" id="led11"></div>
            <div class="led" id="led12"></div><div class="led" id="led13"></div>
            <div class="led" id="led14"></div>
            <div class="led" id="led15"></div><div class="led" id="led16"></div>
            <div class="led" id="led17"></div><div class="led" id="led18"></div>
            <div class="led" id="led19"></div>
            <div class="led" id="led20"></div><div class="led" id="led21"></div>
            <div class="led" id="led22"></div><div class="led" id="led23"></div>
            <div class="led" id="led24"></div>
        </div>
        <div class="color-picker-container">
            <h1 class="color-picker-heading">Color:</h1>
            <input type="color" id="colorPicker" class="color-picker" value="#ffffff">
        </div>
        <button class="roll-dice-btn" id="rollDiceButton"
        onclick="rollDice()">Roll dice</button>
        <button class="roll-dice-btn" id="deepSleepButton" 
        onclick="deepSleep()">Schlafen</button>
    </div>

    <script>
        var colorPicker = document.getElementById('colorPicker');
        var rollDiceButton = document.getElementById('rollDiceButton');
        var r = 255;
        var g = 255;
        var b = 255;
		var leds = [];
        for (var num = 0; num < 25; num++) {
            leds[num] = document.getElementById('led' + num);
            leds[num].dataset.state = 'off';
            leds[num].addEventListener('click', (function(num) {
                return function() {
                    if (this.dataset.state === 'off') {
                        this.style.backgroundColor = colorPicker.value;
                        this.dataset.state = 'on';
                        updateLED(num, true);
                    } else {
                        this.style.backgroundColor = 'black';
                        this.dataset.state = 'off';
                        updateLED(num, false);
                    }
                    
                };
            })(num));
        }

        colorPicker.addEventListener('input', function() {
            this.style.backgroundColor = this.value;
            r = parseInt(colorPicker.value.substring(1, 3), 16);
            g = parseInt(colorPicker.value.substring(3, 5), 16);
            b = parseInt(colorPicker.value.substring(5, 7), 16);
        });

        

        function updateLED(num, power) {
            var xhttp = new XMLHttpRequest();
            xhttp.onreadystatechange = function () {
                if (this.readyState == 4 && this.status == 200) {
                    console.log(this.responseText);
                }
            };
            if (power) {
                xhttp.open("GET", `/update?led=${num}&r=${r}&g=${g}&b=${b}`, true);
            }
            else {
                xhttp.open("GET", `/update?led=${num}&r=0&g=0&b=0`, true);
            }
            xhttp.send();
        }

        function switchColor() {
            var colorHex = colorPicker.value;
            
            var xhttp = new XMLHttpRequest();
            xhttp.onreadystatechange = function() {
                if (this.readyState == 4 && this.status == 200) {
                    console.log(this.responseText);
                }
            };
            xhttp.open("GET", `/color?r=${r}&g=${g}&b=${b}`, true);
            xhttp.send();
        }

        function deepSleep() {

            var xhttp = new XMLHttpRequest();
            xhttp.onreadystatechange = function() {
                if (this.readyState == 4 && this.status == 200) {
                    console.log(this.responseText);
                }
            };
            xhttp.open("GET", "/deepsleep", true);
            xhttp.send();
            
        }

        function rollDice() {
            // Disable the button
            rollDiceButton.disabled = true;
            var roll = Math.floor(Math.random() * 6 + 1);
            // Make the GET request
            var xhttp = new XMLHttpRequest();
            xhttp.onreadystatechange = function() {
                if (this.readyState == 4 && this.status == 200) {
                    console.log(this.responseText);
                }
            };
            xhttp.open("GET", `/rolldice?result=${roll}&r=${r}&g=${g}&b=${b}`, true);
            xhttp.send();
            setTimeout(function() {
                rollDiceButton.disabled = false;   
            }, 2000); 
        }
    </script>
</body>
</html>
)rawliteral"};

void prerolldice(void);
void marius(void);
void wifiSetup(void);
void loadCredentials(void);
void saveCredentials(void);
void printPattern(uint8_t pattern);
void setLED(uint8_t num);
void clearLED(uint8_t num);

void setup()
{
    Serial.begin(9600);
    pinMode(D1, INPUT);
    pinMode(D2, INPUT);
    wifiSetup();
    pixels.begin(); // Initialisierung der NeoPixel
    printPattern(0);
    // Route for root / web page
    webServer.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
                 { request->send_P(200, "text/html", index_html); });

    // GET request to <ESP_IP>/rolldice
    webServer.on("/rolldice", HTTP_GET, [](AsyncWebServerRequest *request)
                 {
    if (request->hasParam(PARAM_R) &&
        request->hasParam(PARAM_G) &&
        request->hasParam(PARAM_B) &&
        request->hasParam(PARAM_RESULT)) {
        r = request->getParam(PARAM_R)->value().toInt();
        g = request->getParam(PARAM_G)->value().toInt();
        b = request->getParam(PARAM_B)->value().toInt();
        roll = request->getParam(PARAM_RESULT)->value().toInt();
        btn = true;
        startMillis = millis();
    }
                    request->send(200, "text/plain", "OK"); });

    // GET request to <ESP_IP>/update
    webServer.on("/update", HTTP_GET, [](AsyncWebServerRequest *request)
                 {
    uint8_t led = 0;
    if (request->hasParam(PARAM_R) &&
        request->hasParam(PARAM_G) &&
        request->hasParam(PARAM_B) &&
        request->hasParam(PARAM_LED)) {
        r = request->getParam(PARAM_R)->value().toInt();
        g = request->getParam(PARAM_G)->value().toInt();
        b = request->getParam(PARAM_B)->value().toInt();
        led = request->getParam(PARAM_LED)->value().toInt();
        setLED(led);
        startMillis = millis();
    }
                    request->send(200, "text/plain", "OK"); });

    // GET request to <ESP_IP>/deepsleep
    webServer.on("/deepsleep", HTTP_GET, [](AsyncWebServerRequest *request)
                 {
    marius();
    request->send(200, "text/plain", "OK"); });

    // Start webServer
    webServer.begin();
}

// Main code
void loop()
{
    if (sleep)
    {
        ESP.deepSleep(0);
    }
    currentMillis = millis();
    d1 = digitalRead(D1);
    if (!d1)
    {
        roll = random(6) + 1;
        btn = true;
    }

    if (btn)
    {
        if (currentMillis - startMillis >= period)
        {
            prerolldice();
            startMillis = currentMillis;
            period += 50;
        }
        if (period == 400)
        {
            printPattern(roll);
            btn = false;
            period = 0;
        }
    }
    if (currentMillis - startMillis >= DEEP_SLEEP)
    {
        marius();
    }
    if (currentMillis - startMillis >= LIGHTS_OFF)
    {
        printPattern(0);
    }
}

void wifiSetup(void)
{
    ESPAsync_WiFiManager wm(&webServer, &dnsServer, "AP_Config");

    if (digitalRead(D2) == LOW)
    {
        wm.startConfigPortal(ssidAP, pwAP);
        ssid = wm.WiFi_SSID();
        pw = wm.WiFi_Pass();
        saveCredentials();
    }
    else
    {
        loadCredentials();
        WiFi.begin(ssid, pw);

        for (int ctr = 0; ctr < 15; ctr++)
        {
            delay(1000);
            Serial.print(".");
            if (WiFi.status() == WL_CONNECTED)
                break;
            if (ctr == 14)
                sleep = true;
        }
        Serial.println(WiFi.localIP());
    }
}

/** Load WLAN credentials from EEPROM */
void loadCredentials()
{
    EEPROM.begin(512);
    EEPROM.get(128, ssid);
    EEPROM.get(128 + sizeof(ssid), pw);
    char ok[2 + 1] = "OK";
    EEPROM.get(128 + sizeof(ssid) + sizeof(pw), ok);
    EEPROM.end();
    if (String(ok) != String("OK"))
    {
        ssid[0] = 0;
        pw[0] = 0;
    }
}

/** Store WLAN credentials to EEPROM */
void saveCredentials()
{
    EEPROM.begin(512);
    EEPROM.put(128, ssid);
    EEPROM.put(128 + sizeof(ssid), pw);
    char ok[2 + 1] = "OK";
    EEPROM.put(128 + sizeof(ssid) + sizeof(pw), ok);
    EEPROM.commit();
    EEPROM.end();
}

void prerolldice(void)
{
    uint8_t preroll = random(6) + 1;
    printPattern(preroll);
    Serial.print("You rolled: ");
    Serial.print(preroll);
    Serial.println();
}

void marius()
{
    printPattern(0);
    ESP.deepSleep(0);
}

void printPattern(uint8_t pattern)
{
    for (int i = 0; i < 25; i++)
    {
        clearLED(i);
    }
    const std::vector<int> &dice = dicePatterns[pattern];
    for (int i : dice)
    {
        setLED(i);
    }
    pixels.show();
}

void setLED(uint8_t num)
{
    pixels.setPixelColor(num, pixels.Color(r, g, b));
}

void clearLED(uint8_t num)
{
    pixels.setPixelColor(num, pixels.Color(0, 0, 0));
}
