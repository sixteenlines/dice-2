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
DNSServer dnsServer;

/* Control vars */
uint8_t roll = 0;
unsigned long startMillis = 0;
unsigned long currentMillis = 0;
unsigned short period = 50;
bool btn = false;
bool sleep = false;
bool manager = false;

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
    if (manager)
    {
        dnsServer.processNextRequest();
    }
    else
    {
        if (sleep)
        {
            marius();
        }
        currentMillis = millis();

        if (!(digitalRead(BTN0_PIN)))
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
}

String readFile(const String path)
{
    Serial.print("Reading file ");

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
    Serial.printf("Writing file ");

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

void initIO(void)
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

void initFS(void)
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
        managerSetup();
        manager = true;
        return true;
    }
    // try to load stored creds
    else if (loadCredentials())
    {
        return clientSetup();
    }
    else
    {
        Serial.print("None or invalid credentials. To set, boot using AP-Config Mode ");
        Serial.println("by pressing S0 and S1.");
        return false;
    }
}

void managerSetup(void)
{
    // Setup AP
    Serial.println("Setting AP (Access Point)");
    WiFi.softAPConfig(IPAddress(8, 8, 8, 8), IPAddress(8, 8, 8, 8), IPAddress(255, 255, 255, 0));
    WiFi.softAP("MAGIC-DICE-SETUP", "noetmandel");

    localIP = WiFi.softAPIP();
    Serial.print("AP IP address: ");
    Serial.println(localIP);
    dnsServer.start(53, "*", localIP);
    hostManager();
}

bool clientSetup(void)
{
    WiFi.mode(WIFI_STA);
    if (creds[_IPAD] == "" || creds[_GATE] == "" || creds[_SUBN] == "")
    {
        Serial.println("No Static config provided, using DHCP.");
    }
    else
    {
        Serial.println("Static config provided.");
        localIP.fromString(creds[_IPAD].c_str());
        localGateway.fromString(creds[_GATE].c_str());
        localSubnet.fromString(creds[_SUBN].c_str());
        if (!WiFi.config(localIP, localGateway, localSubnet))
        {
            Serial.println("Client failed to configure");
            return false;
        }
    }
    WiFi.begin(creds[_SSID].c_str(), creds[_PASS].c_str());
    Serial.println("Connecting to WiFi...");
    for (int ctr = 0; ctr < 21; ctr++)
    {
        delay(500);
        Serial.print(".");
        digitalWrite(STATUSLED_PIN, (ctr % 2));
        if (WiFi.status() == WL_CONNECTED)
        {
            Serial.print("Connected to " + creds[_SSID] + "with IP ");
            Serial.println(WiFi.localIP());
            hostIndex();
            return true;
        }
        if (ctr == 14)
        {
            Serial.println("Couldnt connect with credentials.");
            break;
        }
    }
    return false;
}

/** Load WLAN credentials from FS */
bool loadCredentials(void)
{
    for (int i = 0; i < 5; i++)
    {
        creds[i] = readFile(paths[i]);
        Serial.println(creds[i]);
    }
    if (creds[_SSID] == "")
    {
        Serial.println("Invalid SSID.");
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

void marius(void)
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

void hostManager(void)
{
    webServer.onNotFound([](AsyncWebServerRequest *request)
                         {
    if (!handleFileRequest(request, request->url()))
      request->send(404, "text/plain", "File not found"); });

    webServer.on("/generate_204", [](AsyncWebServerRequest *request)
                 { request->redirect(MANAGER); }); // android captive portal redirect
    webServer.on("/redirect", [](AsyncWebServerRequest *request)
                 { request->redirect(MANAGER); }); // microsoft redirect
    webServer.on("/hotspot-detect.html", [](AsyncWebServerRequest *request)
                 { request->redirect(MANAGER); }); // apple call home
    webServer.on("/mobile/status.php", [](AsyncWebServerRequest *request)
                 { request->redirect(MANAGER); }); // ? call home

    webServer.on("/submit", HTTP_POST, [](AsyncWebServerRequest *request)
                 {
            for (uint8_t i = 0; i < request->params(); i++)
            {
                AsyncWebParameter *p = request->getParam(i);
                if (p->isPost())
                {
                    String paramName = p->name();
                    String paramValue = p->value().c_str();
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
                    creds[writeOffset] = paramValue;
                    Serial.print(message);
                    Serial.println(creds[writeOffset]);
                    writeFile(paths[writeOffset], creds[writeOffset].c_str());
                }
            }

            String response = "Done. ESP will restart.";
            request->send(200, "text/plain", response);
            delay(1500);
            ESP.restart(); });
}

bool handleFileRequest(AsyncWebServerRequest *request, String path)
{
    Serial.println("handleFileRead: " + path);
    String contentType;
    if (path.endsWith("/"))
        path = "manager.html";
    if (path.endsWith(".html"))
        contentType = "text/html";
    if (path.endsWith(".css"))
        contentType = "text/css";
    if (LittleFS.exists(path))
    {
        Serial.println("request: " + path);
        AsyncWebServerResponse *response = request->beginResponse(LittleFS, path, contentType);
        request->send(response);
        return true;
    }
    return false;
}