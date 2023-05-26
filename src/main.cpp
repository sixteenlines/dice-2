#include <Adafruit_NeoPixel.h>
#include <ESP8266WiFi.h>
#include <ESPAsyncWebServer.h>
#include <ESPAsyncTCP.h>
#include <ESPAsync_WiFiManager.h>
#include <ESP8266WiFiMulti.h>
#include <EEPROM.h>
#include <Arduino.h>

// Pins
#define PIN 2
#define NUMPIXELS 25
Adafruit_NeoPixel pixels = Adafruit_NeoPixel(NUMPIXELS, PIN, NEO_RGB + NEO_KHZ400);

/* Endpoint params */
const char *PARAM_INPUT_COLORR = "r";
const char *PARAM_INPUT_COLORG = "g";
const char *PARAM_INPUT_COLORB = "b";
const char *PARAM_INPUT_RESULT = "result";

/* EEPROM */

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
            <div class="led" id="led1"></div><div class="led" id="led2"></div><div class="led" id="led3"></div><div class="led" id="led4"></div><div class="led" id="led5"></div>
            <div class="led" id="led6"></div><div class="led" id="led7"></div><div class="led" id="led8"></div><div class="led" id="led9"></div><div class="led" id="led10"></div>
            <div class="led" id="led11"></div><div class="led" id="led12"></div><div class="led" id="led13"></div><div class="led" id="led14"></div><div class="led" id="led15"></div>
            <div class="led" id="led16"></div><div class="led" id="led17"></div><div class="led" id="led18"></div><div class="led" id="led19"></div><div class="led" id="led20"></div>
            <div class="led" id="led21"></div><div class="led" id="led22"></div><div class="led" id="led23"></div><div class="led" id="led24"></div><div class="led" id="led25"></div>
        </div>
        <div class="color-picker-container">
            <h1 class="color-picker-heading">Color:</h1>
            <input type="color" id="colorPicker" class="color-picker" value="#ffffff">
        </div>
        <button class="roll-dice-btn" id="rollDiceButton" onclick="rollDice()">Roll dice</button>
        <button class="roll-dice-btn" id="deepSleepButton" onclick="deepSleep()">Schlafen</button>
    </div>

    <script>
        var colorPicker = document.getElementById('colorPicker');
        var rollDiceButton = document.getElementById('rollDiceButton');
		var leds = [];
        for (var i = 1; i <= 25; i++) {
            leds[i] = document.getElementById('led' + i);
        }

        colorPicker.addEventListener('input', function() {
            this.style.backgroundColor = this.value;
            switchColor();
        });

        function switchColor() {
            var colorHex = colorPicker.value;
            var r = parseInt(colorHex.substring(1, 3), 16);
            var g = parseInt(colorHex.substring(3, 5), 16);
            var b = parseInt(colorHex.substring(5, 7), 16);
            
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
            xhttp.open("GET", `/rolldice?result=${roll}`, true);
            xhttp.send();
            setTimeout(function() {
                rollDiceButton.disabled = false;
            }, 2000); 
        }
    </script>
</body>
</html>
)rawliteral"};

void printNumPixel(uint8_t num);
void prerolldice(void);
void marius(void);
void wifiSetup(void);
void loadCredentials(void);
void saveCredentials(void);

void setup()
{
    Serial.begin(9600);
    pinMode(D1, INPUT);
    pinMode(D2, INPUT);
    wifiSetup();
    pixels.begin(); // Initialisierung der NeoPixel
    printNumPixel(0);
    // Route for root / web page
    webServer.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
                 { request->send_P(200, "text/html", index_html); });

    // GET request to <ESP_IP>/rolldice
    webServer.on("/rolldice", HTTP_GET, [](AsyncWebServerRequest *request)
                 {
    String inputResult;
    if (request->hasParam(PARAM_INPUT_RESULT)) {
        inputResult = request->getParam(PARAM_INPUT_RESULT)->value();
        roll = inputResult.toInt();
        btn = true;
	    startMillis = millis();

    }
    else {
        inputResult = "No message sent";
    }
    request->send(200, "text/plain", "OK"); });

    // GET request to <ESP_IP>/deepsleep
    webServer.on("/deepsleep", HTTP_GET, [](AsyncWebServerRequest *request)
                 {
    marius();
    request->send(200, "text/plain", "OK"); });

    // GET request to <ESP_IP>/color?r=x&g=y&b=z
    webServer.on("/color", HTTP_GET, [](AsyncWebServerRequest *request)
                 {
    String inputColorR;
    String inputColorG;
    String inputColorB;
    if (request->hasParam(PARAM_INPUT_COLORR) && request->hasParam(PARAM_INPUT_COLORG) && request->hasParam(PARAM_INPUT_COLORB)) {
        inputColorR = request->getParam(PARAM_INPUT_COLORR)->value();
        inputColorG = request->getParam(PARAM_INPUT_COLORG)->value();
        inputColorB = request->getParam(PARAM_INPUT_COLORB)->value();
        r = inputColorR.toInt();
        g = inputColorG.toInt();
        b = inputColorB.toInt();
        printNumPixel(roll); 
        startMillis = millis();
    }
    else {
      inputColorR = "No message sent";
      inputColorG = "No message sent";
      inputColorB = "No message sent";
    }
    request->send(200, "text/plain", "OK"); });

    // Start webServer
    webServer.begin();
}

// Main code
void loop()
{
    currentMillis = millis();
    d1 = digitalRead(D1);
    if (!d1)
    {
        roll = random(6) + 1;
        btn = true;
    }

    if (btn)
    {
        if (currentMillis - startMillis >= period) // test whether the period has elapsed
        {
            prerolldice();
            startMillis = currentMillis;
            period += 50;
        }
        if (period == 400)
        {
            printNumPixel(roll);
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
        printNumPixel(0);
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
        Serial.println("saving creds");
        Serial.println(ssid);
        Serial.println(pw);
        saveCredentials();
        loadCredentials();
        Serial.println("loading creds");
        Serial.println(ssid);
        Serial.println(pw);
    }
    else
    {
        loadCredentials();
        Serial.println("loading creds");
        Serial.println(ssid);
        Serial.println(pw);
        //     WiFi.begin(ssid, pw);
        //     int ctr = 0;
        //     while (WiFi.status() != WL_CONNECTED)
        //     {
        //         ctr++;
        //         delay(1000);
        //         Serial.print(".");
        //         if (ctr == 15)
        //         {
        //             ESP.deepSleep(0);
        //         }
        //     }
        //     // Print out IP Address
        //     Serial.println(WiFi.localIP());
        // }
        // Serial.println(WiFi.localIP());
        // ssid = (char *)wm.WiFi_SSID().c_str();
        // pw = (char *)wm.WiFi_Pass().c_str();
        // saveCredentials();
        // Serial.println("saved creds");
        // Serial.println();
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
    printNumPixel(preroll);
    Serial.print("You rolled: ");
    Serial.print(preroll);
    Serial.println();
}

void marius()
{
    printNumPixel(0);
    ESP.deepSleep(0);
}

void printNumPixel(uint8_t num)
{
    switch (num)
    {
    case 0:
        // All LEDs LOW
        pixels.setPixelColor(0, pixels.Color(0, 0, 0));
        pixels.setPixelColor(4, pixels.Color(0, 0, 0));
        pixels.setPixelColor(10, pixels.Color(0, 0, 0));
        pixels.setPixelColor(12, pixels.Color(0, 0, 0));
        pixels.setPixelColor(14, pixels.Color(0, 0, 0));
        pixels.setPixelColor(20, pixels.Color(0, 0, 0));
        pixels.setPixelColor(24, pixels.Color(0, 0, 0));
        pixels.show();
        break;
    case 1:
        pixels.setPixelColor(0, pixels.Color(0, 0, 0));
        pixels.setPixelColor(4, pixels.Color(0, 0, 0));
        pixels.setPixelColor(10, pixels.Color(0, 0, 0));
        pixels.setPixelColor(12, pixels.Color(r, g, b));
        pixels.setPixelColor(14, pixels.Color(0, 0, 0));
        pixels.setPixelColor(20, pixels.Color(0, 0, 0));
        pixels.setPixelColor(24, pixels.Color(0, 0, 0));
        pixels.show();
        break;
    case 2:
        pixels.setPixelColor(0, pixels.Color(0, 0, 0));
        pixels.setPixelColor(4, pixels.Color(r, g, b));
        pixels.setPixelColor(10, pixels.Color(0, 0, 0));
        pixels.setPixelColor(12, pixels.Color(0, 0, 0));
        pixels.setPixelColor(14, pixels.Color(0, 0, 0));
        pixels.setPixelColor(20, pixels.Color(r, g, b));
        pixels.setPixelColor(24, pixels.Color(0, 0, 0));
        pixels.show();
        break;
    case 3:
        pixels.setPixelColor(0, pixels.Color(0, 0, 0));
        pixels.setPixelColor(4, pixels.Color(r, g, b));
        pixels.setPixelColor(10, pixels.Color(0, 0, 0));
        pixels.setPixelColor(12, pixels.Color(r, g, b));
        pixels.setPixelColor(14, pixels.Color(0, 0, 0));
        pixels.setPixelColor(20, pixels.Color(r, g, b));
        pixels.setPixelColor(24, pixels.Color(0, 0, 0));
        pixels.show();
        break;
    case 4:
        pixels.setPixelColor(0, pixels.Color(r, g, b));
        pixels.setPixelColor(4, pixels.Color(r, g, b));
        pixels.setPixelColor(10, pixels.Color(0, 0, 0));
        pixels.setPixelColor(12, pixels.Color(0, 0, 0));
        pixels.setPixelColor(14, pixels.Color(0, 0, 0));
        pixels.setPixelColor(20, pixels.Color(r, g, b));
        pixels.setPixelColor(24, pixels.Color(r, g, b));
        pixels.show();
        break;
    case 5:
        pixels.setPixelColor(0, pixels.Color(r, g, b));
        pixels.setPixelColor(4, pixels.Color(r, g, b));
        pixels.setPixelColor(10, pixels.Color(0, 0, 0));
        pixels.setPixelColor(12, pixels.Color(r, g, b));
        pixels.setPixelColor(14, pixels.Color(0, 0, 0));
        pixels.setPixelColor(20, pixels.Color(r, g, b));
        pixels.setPixelColor(24, pixels.Color(r, g, b));
        pixels.show();
        break;
    case 6:
        pixels.setPixelColor(0, pixels.Color(r, g, b));
        pixels.setPixelColor(4, pixels.Color(r, g, b));
        pixels.setPixelColor(10, pixels.Color(r, g, b));
        pixels.setPixelColor(12, pixels.Color(0, 0, 0));
        pixels.setPixelColor(14, pixels.Color(r, g, b));
        pixels.setPixelColor(20, pixels.Color(r, g, b));
        pixels.setPixelColor(24, pixels.Color(r, g, b));
        pixels.show();
        break;
    }
}