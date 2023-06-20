#include <header_main.hpp>
/* Credentials in RAM */
String creds[5] = {
    "", // SSID
    "", // Password
    "", // IP
    "", // Gateway
    ""  // Subnet
};

/* Timer values */
unsigned long deep_sleep = 120000;
unsigned long led_sleep = 20000;

/* Initializing objects*/
Adafruit_NeoPixel pixels =
    Adafruit_NeoPixel(NUM_LEDS, LEDS_PIN, NEO_RGB + NEO_KHZ400);
AsyncWebServer webServer(80);
IPAddress localIP;
IPAddress localGateway;
IPAddress localSubnet;
DNSServer dnsServer;
std::vector<led> ledgrid;
JSONVar settings;

/* Control vars */
uint8_t roll = 0;
unsigned long lastActionTime = 0;
unsigned long currentTime = 0;
unsigned short period = 50;
bool btn = false;
bool sleep = false;
bool manager = false;
uint8_t diceRed = 45;
uint8_t diceGreen = 45;
uint8_t diceBlue = 45;

/*############################*/
/*########## SETUP ###########*/
/*############################*/
void setup()
{
    Serial.begin(115200);
    Serial.println();
    initFS();
    initIO();
    initLeds();
    initSettings();

    if (initWifi())
    {
        webServer.begin();
    }
    else
    {
        sleep = true;
    }
}

/*#################################*/
/*############ MAIN LOOP ##########*/
/*#################################*/
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
        currentTime = millis();

        if (!(digitalRead(BTN0_PIN)))
        {
            roll = random(6) + 1;
            btn = true;
        }

        if (btn)
        {
            if (currentTime - lastActionTime >= period)
            {
                prerolldice();
                lastActionTime = currentTime;
                period += 50;
            }
            if (period == 400)
            {
                printPattern(roll, diceRed, diceGreen, diceBlue);
                btn = false;
                period = 0;
            }
        }
        if (currentTime - lastActionTime >= deep_sleep)
        {
            marius();
        }
        if (currentTime - lastActionTime >= led_sleep)
        {
            hideLEDS();
        }
    }
}

/*########################################*/
/*###########  INIT FUNCTIONS  ###########*/
/*########################################*/

void initLeds(void)
{
    for (int i = 0; i < NUM_LEDS; i++)
    {
        ledgrid.push_back(led());
    }

    pixels.begin();
    showLEDS();
    Serial.println("LED init success");
}

void initIO(void)
{
    pinMode(BTN0_PIN, INPUT);
    pinMode(BTN1_PIN, INPUT);
    pinMode(BTN2_PIN, INPUT_PULLUP);
    pinMode(RETENTION_PIN, OUTPUT);
    pinMode(STATUSLED_PIN, OUTPUT);
    digitalWrite(RETENTION_PIN, HIGH);
    Serial.println("IO init success");
}

void initFS(void)
{
    if (!LittleFS.begin())
    {
        Serial.println("An error has occurred while mounting LittleFS");
    }
    Serial.println("LittleFS mounted successfully");
}

void initSettings(void)
{
    if (!(readFile(paths[_DEVICE_TO]).toInt() == 0))
    {
        deep_sleep = readFile(paths[_DEVICE_TO]).toInt();
        Serial.println(deep_sleep);
        led_sleep = readFile(paths[_LED_TO]).toInt();
        Serial.println(led_sleep);
    }
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

/*############################################*/
/*##########  WIFI CONFIG / SETUP  ###########*/
/*############################################*/
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
    for (int ctr = 0, tries = 0; !(WiFi.status() == WL_CONNECTED); ctr++)
    {
        if (ctr > 0)
            setLED(patterns[LOADING][ctr - 1], 0, 0, 0);
        if (ctr == 15)
        {
            ctr = 0;
            tries++;
            setLED(patterns[LOADING][15], 0, 0, 0);
        }
        setLED(patterns[LOADING][ctr], 0, 100, 150);
        showLEDS();
        delay(100);

        if (tries == 3)
        {
            printPattern(BIGDOT, 255, 0, 0);
            delay(100);
            hideLEDS();
            Serial.println("Couldnt connect with credentials.");
            return false;
        }
    }
    Serial.print("Connected to " + creds[_SSID] + "with IP ");
    Serial.println(WiFi.localIP());
    printPattern(BIGDOT, 0, 255, 0);
    delay(150);
    printPattern(0);
    hostIndex();
    return true;
}

/*################################################*/
/*##########  DICE-RANDOMIZER FUNCTION  ##########*/
/*################################################*/
void prerolldice(void)
{
    uint8_t preroll = random(6) + 1;
    printPattern(preroll, diceRed, diceGreen, diceBlue);
    Serial.print("You rolled: ");
    Serial.print(preroll);
    Serial.println();
}

/*######################################*/
/*##########  SLEEP FUNCTION  ##########*/
/*######################################*/
void marius(void)
{
    digitalWrite(RETENTION_PIN, LOW);
}

/*########################################*/
/*########## WEBSITE HOSTING #############*/
/*########################################*/
void hostIndex(void)
{
    // Route for root / web page
    webServer.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
                 { request->send(LittleFS, "/index.html",
                                 "text/html", false); });
    webServer.serveStatic("/", LittleFS, "/");

    // POST request to <ESP_IP>/rolldice
    webServer.on("/rolldice", HTTP_POST, [](AsyncWebServerRequest *request)
                 {
    if (request->hasParam(PARAM_R, true, false) &&
        request->hasParam(PARAM_G, true, false) &&
        request->hasParam(PARAM_B, true, false) &&
        request->hasParam(PARAM_RESULT, true, false)) {
        diceRed = request->getParam(PARAM_R, true, false)->value().toInt();
        diceGreen = request->getParam(PARAM_G, true, false)->value().toInt();
        diceBlue = request->getParam(PARAM_B, true, false)->value().toInt();
        roll = request->getParam(PARAM_RESULT, true, false)->value().toInt();
        btn = true;
        lastActionTime = millis();
    }
    request->send(200, "text/plain", "OK"); });

    // POST request to <ESP_IP>/update
    webServer.on("/update", HTTP_POST, [](AsyncWebServerRequest *request)
                 {
    if (request->hasParam(PARAM_R, true, false) &&
        request->hasParam(PARAM_G, true, false) &&
        request->hasParam(PARAM_B, true, false) &&
        request->hasParam(PARAM_LED, true, false)) {
        uint8_t num = request->getParam(PARAM_LED, true, false)->value().toInt();
        uint8_t r = request->getParam(PARAM_R, true, false)->value().toInt();
        uint8_t g = request->getParam(PARAM_G, true, false)->value().toInt();
        uint8_t b = request->getParam(PARAM_B, true, false)->value().toInt();
        setLED(num, r, g, b);
        showLEDS();
        lastActionTime = millis();
    }
    request->send(200, "text/plain", "OK"); });

    // clear LED grid
    webServer.on("/clear", HTTP_GET, [](AsyncWebServerRequest *request)
                 {
        printPattern(0);
        request->send(200, "text/plain", "OK"); });

    // GET request settings page
    webServer.on("/settings", HTTP_GET, [](AsyncWebServerRequest *request)
                 { request->send(LittleFS, "/settings.html", "text/html", false); });
    webServer.serveStatic("/", LittleFS, "/");

    // Request for the latest sensor readings
    webServer.on("/loadsettings", HTTP_GET, [](AsyncWebServerRequest *request)
                 {
        String json = getSettings();
        request->send(200, "application/json", json);
        json = String(); });

    // POST request to save settings
    webServer.on("/savesettings", HTTP_POST, [](AsyncWebServerRequest *request)
                 {
    if (request->hasParam(PARAM_DEVICE_TIMEOUT, true, false) &&
        request->hasParam(PARAM_LED_TIMEOUT, true, false) ) {
        deep_sleep = request->getParam(PARAM_DEVICE_TIMEOUT, true, false)->value().toInt() *1000;
        led_sleep = request->getParam(PARAM_LED_TIMEOUT, true, false)->value().toInt() *1000;
    }   writeFile("/settings/devicetimeout.txt", std::to_string(deep_sleep).c_str());
        writeFile("/settings/ledtimeout.txt", std::to_string(led_sleep).c_str());
    
    request->send(200, "text/plain", "OK"); });

    // GET request to sleep
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
            printPattern(BIGDOT, 0, 255, 255);
            delay(150);
            printPattern(0);
            ESP.restart(); });
}

/*#############################################*/
/*##########  FILE SYSTEM FUNCTIONS  ##########*/
/*#############################################*/
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

String getSettings()
{
    settings["ledtimeout"] = String(led_sleep / 1000);
    settings["devicetimeout"] = String(deep_sleep / 1000);
    String jsonString = JSON.stringify(settings);
    return jsonString;
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

/*###################################*/
/*########## LED functions ##########*/
/*###################################*/
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

void setLED(uint8_t num, uint8_t r, uint8_t g, uint8_t b)
{
    ledgrid[num].r = r;
    ledgrid[num].g = g;
    ledgrid[num].b = b;
}

void hideLEDS()
{
    for (int num = 0; num < NUM_LEDS; num++)
    {
        pixels.setPixelColor(num, pixels.Color(0, 0, 0));
    }
    pixels.show();
}

void showLEDS(void)
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