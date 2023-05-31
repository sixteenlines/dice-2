#include <main.hpp>

/* Credentials in RAM */
String creds[5] = {
    "", // SSID
    "", // Password
    "", // IP
    "", // Gateway
    ""  // Subnet
};

/* Initializing objects*/
Adafruit_NeoPixel pixels =
    Adafruit_NeoPixel(NUMPIXELS, LEDS_PIN, NEO_RGB + NEO_KHZ400);
AsyncWebServer webServer(80);
IPAddress localIP;
IPAddress localGateway;
IPAddress localSubnet;

/* Control vars */
uint8_t roll = 0;
unsigned long startMillis = 0;
unsigned long currentMillis = 0;
unsigned short period = 50;
bool btn = false;
bool d1 = true;
bool sleep = false;

/* Colors */
uint8_t r = 255;
uint8_t g = 255;
uint8_t b = 255;

void setup()
{
    Serial.begin(115200);
    initFS();
    initIO();
    if (initWifi())
    {
        webServer.begin();
    }
    else
    {
        sleep = true;
    }
}

// Main code
void loop()
{
    if (sleep)
    {
        marius();
    }
    currentMillis = millis();
    d1 = digitalRead(BTN0_PIN);
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

String readFile(const String path)
{
    Serial.printf("Reading file: %s\r\n", path);

    File file = LittleFS.open(path, "r");
    if (!file || file.isDirectory())
    {
        Serial.println("- failed to open file for reading");
        return String();
    }

    String fileContent;
    while (file.available())
    {
        fileContent = file.readStringUntil('\n');
        break;
    }

    file.close();
    return fileContent;
}

void writeFile(const String path, const char *message)
{
    Serial.printf("Writing file: %s\r\n", path);

    File file = LittleFS.open(path, "w");
    if (!file)
    {
        Serial.println("- failed to open file for writing");
        return;
    }

    if (file.print(message))
    {
        Serial.println("- file written");
    }
    else
    {
        Serial.println("- write failed");
    }

    file.close();
}

void initIO()
{
    pinMode(BTN0_PIN, INPUT);
    pinMode(BTN1_PIN, INPUT);
    pinMode(BTN2_PIN, INPUT_PULLUP);
    pinMode(RETENTION_PIN, OUTPUT);
    pinMode(STATUSLED_PIN, OUTPUT);
    digitalWrite(RETENTION_PIN, HIGH);
    pixels.begin();
    printPattern(0);
}

void initFS()
{
    if (!LittleFS.begin())
    {
        Serial.println("An error has occurred while mounting LittleFS");
    }
    Serial.println("LittleFS mounted successfully");
}

bool initWifi(void)
{
    // Wifi manager boot requested
    if (digitalRead(BTN1_PIN) == LOW)
    {
        return managerSetup();
    }
    // try to load stored creds
    else if (loadCredentials)
    {
        return clientSetup();
    }
    else
    {
        Serial.print("No Credentials saved. To set, boot using AP-Config Mode ");
        Serial.println("by pressing S0 and S1.");
        return false;
    }
}

bool managerSetup()
{
    // Setup AP
    Serial.println("Setting AP (Access Point)");
    WiFi.softAP("MAGIC-DICE-SETUP", "noetmandel");

    IPAddress IP = WiFi.softAPIP();
    Serial.print("AP IP address: ");
    Serial.println(IP);
}

bool clientSetup()
{
    WiFi.mode(WIFI_STA);
    localIP.fromString(creds[2].c_str());
    localGateway.fromString(creds[3].c_str());
    localSubnet.fromString(creds[4].c_str());
    for (int ctr = 0; ctr < 21; ctr++)
    {
        delay(500);
        Serial.print(".");
        digitalWrite(STATUSLED_PIN, (ctr % 2));
        if (WiFi.status() == WL_CONNECTED)
        {
            Serial.print("Connected to " + creds[0] + "with IP ");
            Serial.println(WiFi.localIP());
            hostIndex();
            return true;
        }
        if (ctr == 14)
        {
            Serial.println("Couldnt connect with credentials.");
            return false;
        }
    }
}

/** Load WLAN credentials from EEPROM */
bool loadCredentials()
{
    for (int i = 0; i < 5; i++)
    {
        creds[i] = readFile(paths[i]);
        Serial.println(creds[i]);
    }
    if (creds[_SSID] == "" || creds[_IPAD] == "")
    {
        Serial.println("Invalid SSID or IP address.");
        return false;
    }
    else
        return true;
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
    digitalWrite(RETENTION_PIN, LOW);
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

void hostIndex(void)
{
    // Route for root / web page
    webServer.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
                 { request->send(LittleFS, "/index.html",
                                 "text/html", false); });
    webServer.serveStatic("/", LittleFS, "/");

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
            pixels.show();
            startMillis = millis();
        }
        request->send(200, "text/plain", "OK"); });

    // GET request to <ESP_IP>/deepsleep
    webServer.on("/deepsleep", HTTP_GET, [](AsyncWebServerRequest *request)
                 {
        sleep = true;
        request->send(200, "text/plain", "OK"); });
}

void hostManager()
{
    webServer.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
                 { request->send(LittleFS, "/manager.html", "text/html"); });

    webServer.serveStatic("/", LittleFS, "/");

    webServer.on("/", HTTP_POST, [](AsyncWebServerRequest *request)
                 {
        for (int i = 0; i < request->params(); i++)
        {
            AsyncWebParameter *p = request->getParam(i);
            if (p->isPost())
            {
                String paramName = p->name();
                String message;
                uint8_t writeOffset;

                if (paramName == PARAM_INPUT_0)
                {
                    writeOffset = _SSID;
                    message = "SSID set to: ";
                }
                else if (paramName == PARAM_INPUT_1)
                {
                    writeOffset = _PASS;
                    message = "Password set to: ";
                }
                else if (paramName == PARAM_INPUT_2)
                {
                    writeOffset = _IPAD;
                    message = "IP Address set to: ";
                }
                else if (paramName == PARAM_INPUT_3)
                {
                    writeOffset = _GATE;
                    message = "Gateway set to: ";
                }
                else if (paramName == PARAM_INPUT_4)
                {
                    writeOffset = _SUBN;
                    message = "Subnet mask set to: ";
                }
                else
                {
                    continue; // Skip unrecognized parameters
                }

                Serial.print(message);
                Serial.println(creds[writeOffset]);
                writeFile(paths[writeOffset], creds[writeOffset].c_str());
            }
        }

        String response = "Done. ESP will restart, connect to your router and go to IP address: " + creds[_IPAD];
        request->send(200, "text/plain", response);
        delay(1500);
        ESP.restart(); });
}
