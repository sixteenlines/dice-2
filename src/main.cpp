#include <header_main.hpp>
/* Credentials in RAM */
String creds[5] = {
    "", // SSID
    "", // Password
    "", // IP
    "", // Gateway
    ""  // Subnet
};

/* Timeout default values */
unsigned long deep_sleep = 120000;
unsigned long led_sleep = 20000;

/* Initializing objects*/
Adafruit_NeoPixel pixels =
    Adafruit_NeoPixel(NUM_LEDS, LEDS_PIN, NEO_GRB + NEO_KHZ400);
AsyncWebServer webServer(80);
IPAddress localIP;
IPAddress localGateway;
IPAddress localSubnet;
DNSServer dnsServer;
std::vector<led> ledgrid;
JSONVar settings;

/* Control vars */
uint8_t dieRollResult = 0;
unsigned long lastActionTime = 0;
unsigned long currentTime = 0;
bool rollRequested = false;
bool sleepRequested = false;
bool managerRequested = false;
uint8_t diceRed = 120;
uint8_t diceGreen = 120;
uint8_t diceBlue = 120;

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
        webServer.begin();
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
        dnsServer.processNextRequest();
    }
    else
    {
        if (sleepRequested)
        {
            marius();
        }
        currentTime = millis();

        if (!(digitalRead(BTN0_PIN)))
        {
            dieRollResult = random(6) + 1;
            rollRequested = true;
        }

        if (rollRequested)
        {
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
            marius();
        }
        if (currentTime - lastActionTime >= led_sleep)
        {
            hideLEDS();
        }
    }
}

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

void initIO()
{
    Serial.begin(115200);
    Serial.println();
    pinMode(BTN0_PIN, INPUT);
    pinMode(BTN1_PIN, INPUT);
    pinMode(BTN2_PIN, INPUT);
    pinMode(RETENTION_PIN, OUTPUT);
    pinMode(STATUSLED_PIN, OUTPUT);
    digitalWrite(RETENTION_PIN, HIGH);
    Serial.println("[\e[0;32m  OK  \e[0;37m] Initializing system I/O");
}

void initFS()
{
    if (LittleFS.begin())
        Serial.println("[\e[0;32m  OK  \e[0;37m] Initializing file system");
    else
        Serial.println("[\e[0;31mFAILED\e[0;37m] Initializing file system");
}
// Tries to read timeout settings from flash
void initSettings()
{
    if (!(readFile(paths[_DEVICE_TO]).toInt() == 0))
    {
        deep_sleep = readFile(paths[_DEVICE_TO]).toInt();
        led_sleep = readFile(paths[_LED_TO]).toInt();
        Serial.println("[\e[0;32m  OK  \e[0;37m] Loading settings");
    }
    else
    {
        Serial.println("[\e[0;31mFAILED\e[0;37m] Loading settings");
        Serial.println(INDENT + "Loading default:");
    }
    Serial.print("         Device timeout: ");
    Serial.println(deep_sleep);
    Serial.print("         LED timeout: ");
    Serial.println(led_sleep);
}

bool initWifi()
{
    // Wifi manager boot requested
    if (digitalRead(BTN1_PIN) == LOW)
    {
        managerSetup();
        managerRequested = true;
        return true;
    }
    // try to load stored creds
    else if (loadCredentials())
    {
        return clientSetup();
    }
    else
    {
        return false;
    }
}

// Setup Access Point mode with wifi manager
void managerSetup()
{
    Serial.println("[\e[0;32m  OK  \e[0;37m] Requesting Access Point");
    WiFi.softAPConfig(IPAddress(8, 8, 8, 8), IPAddress(8, 8, 8, 8),
                      IPAddress(255, 255, 255, 0));
    WiFi.softAP(AP_SSID, AP_PW);
    Serial.print("         ");
    Serial.println("Connect to: " + AP_SSID + " with password: " + AP_PW);
    localIP = WiFi.softAPIP();
    dnsServer.start(53, "*", localIP);
    hostManager();
}

// Setup station mode with credentials in ram
bool clientSetup()
{
    WiFi.mode(WIFI_STA);
    if (!(creds[_IPAD] == "" || creds[_GATE] == "" || creds[_SUBN] == ""))
    {
        localIP.fromString(creds[_IPAD].c_str());
        localGateway.fromString(creds[_GATE].c_str());
        localSubnet.fromString(creds[_SUBN].c_str());
        if (WiFi.config(localIP, localGateway, localSubnet))
        {
            Serial.println("[\e[0;32m  OK  \e[0;37m] Setting WiFi Config");
        }
        else
        {
            Serial.println("[\e[0;31mFAILED\e[0;37m] Setting WiFi Config");
            return false;
        }
    }
    Serial.print(INDENT + "Connecting to WiFi");
    WiFi.begin(creds[_SSID].c_str(), creds[_PASS].c_str());

    for (int ctr = 0, tries = 0; !(WiFi.status() == WL_CONNECTED); ctr++)
    {
        // print loading square and check wifi connection
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
        Serial.print(".");

        if (tries == 3) // after 3 loading cycles, blink red dot and return
        {
            printPattern(BIGDOT, 255, 0, 0);
            delay(100);
            hideLEDS();
            Serial.println();
            Serial.println("[\e[0;31mFAILED\e[0;37m] Connecting to WiFi");
            return false;
        }
    }
    printPattern(BIGDOT, 0, 255, 0); // success = green dot
    delay(150);
    printPattern(0);
    hostIndex();
    Serial.println();
    Serial.println("[\e[0;32m  OK  \e[0;37m] Connecting to WiFi");
    return true;
}

// prints a random die roll to the matrix
void prerolldice()
{
    uint8_t preroll = random(6) + 1;
    printPattern(preroll, diceRed, diceGreen, diceBlue);
}

// pulls self-retention pin low, device turns off
void marius()
{
    digitalWrite(RETENTION_PIN, LOW);
}

/*############################## WEBSITE STUFF ##############################*/
void hostIndex()
{
    // Route for root / web page
    webServer.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
                 { request->send(LittleFS, "/index.html",
                                 "text/html", false); });
    webServer.serveStatic("/", LittleFS, "/");

    // Answer POST request to /rolldice
    webServer.on("/rolldice", HTTP_POST, [](AsyncWebServerRequest *request)
                 {
    if (request->hasParam(PARAM_R, true, false) &&
        request->hasParam(PARAM_G, true, false) &&
        request->hasParam(PARAM_B, true, false) &&
        request->hasParam(PARAM_RESULT, true, false)) {
        diceRed = request->getParam(PARAM_R, true, false)->value().toInt();
        diceGreen = request->getParam(PARAM_G, true, false)->value().toInt();
        diceBlue = request->getParam(PARAM_B, true, false)->value().toInt();
        dieRollResult = request->getParam(PARAM_RESULT, true, false)
                                ->value().toInt();
        rollRequested = true;
        lastActionTime = millis();
    }
    request->send(200, "text/plain", "OK"); });

    // Answer POST request to /update
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

    // Answer GET request to /clear
    webServer.on("/clear", HTTP_GET, [](AsyncWebServerRequest *request)
                 {
        printPattern(0);
        request->send(200, "text/plain", "OK"); });

    // Answer GET request /settings
    webServer.on("/settings", HTTP_GET, [](AsyncWebServerRequest *request)
                 { request->send(LittleFS, "/settings.html", "text/html",
                                 false); });
    webServer.serveStatic("/", LittleFS, "/");

    // Answer GET request /loadsettings
    webServer.on("/loadsettings", HTTP_GET, [](AsyncWebServerRequest *request)
                 {
        String json = getSettings();
        request->send(200, "application/json", json);
        json = String(); });

    // Answer POST request to save settings
    webServer.on("/savesettings", HTTP_POST, [](AsyncWebServerRequest *request)
                 {
    if (request->hasParam(PARAM_DEVICE_TIMEOUT, true, false) &&
        request->hasParam(PARAM_LED_TIMEOUT, true, false) ) {
        deep_sleep = request->getParam(PARAM_DEVICE_TIMEOUT, true, false)
                            ->value().toInt() *1000;
        led_sleep = request->getParam(PARAM_LED_TIMEOUT, true, false)
                            ->value().toInt() *1000;
    }   writeFile("/settings/devicetimeout.txt", 
                    std::to_string(deep_sleep).c_str());
        writeFile("/settings/ledtimeout.txt", 
                    std::to_string(led_sleep).c_str());
    
    request->send(200, "text/plain", "OK"); });

    // Answer GET request to /deepsleep
    webServer.on("/deepsleep", HTTP_GET, [](AsyncWebServerRequest *request)
                 {
        sleepRequested = true;
        request->send(200, "text/plain", "OK"); });
}

void hostManager()
{
    webServer.onNotFound([](AsyncWebServerRequest *request)
                         {
    if (!handleFileRequest(request, request->url()))
      request->send(404, "text/plain", "File not found"); });

    // Catch for various captive portal default redirects
    webServer.on("/generate_204", [](AsyncWebServerRequest *request)
                 { request->redirect(MANAGER); }); // android captive portal
    webServer.on("/redirect", [](AsyncWebServerRequest *request)
                 { request->redirect(MANAGER); }); // microsoft redirect
    webServer.on("/hotspot-detect.html", [](AsyncWebServerRequest *request)
                 { request->redirect(MANAGER); }); // apple call home
    webServer.on("/mobile/status.php", [](AsyncWebServerRequest *request)
                 { request->redirect(MANAGER); }); // various call home

    webServer.on("/submit", HTTP_POST, [](AsyncWebServerRequest *request)
                 {
            for (uint8_t i = 0; i < request->params(); i++)
            {
                AsyncWebParameter *p = request->getParam(i);
                if (p->isPost())
                {
                    String paramName = p->name();
                    String paramValue = p->value().c_str();
                    uint8_t writeOffset;

                    if (paramName == PARAM_INPUT_0)
                    {
                        writeOffset = _SSID;
                    }
                    else if (paramName == PARAM_INPUT_1)
                    {
                        writeOffset = _PASS;
                    }
                    else if (paramName == PARAM_INPUT_2)
                    {
                        writeOffset = _IPAD;
                    }
                    else if (paramName == PARAM_INPUT_3)
                    {
                        writeOffset = _GATE;
                    }
                    else if (paramName == PARAM_INPUT_4)
                    {
                        writeOffset = _SUBN;
                    }
                    else
                    {
                        continue; // Skip unrecognized parameters
                    }
                    creds[writeOffset] = paramValue;
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

/*############################## FILESYSTEM STUFF ###########################*/

String readFile(const String path)
{
    Serial.println(INDENT + "Reading file at " + path);
    File file = LittleFS.open(path, "r");
    if (!file || file.isDirectory())
    {
        Serial.println("[\e[0;31mFAILED\e[0;37m] Reading file");
        return String();
    }

    String fileContent;
    while (file.available())
    {
        fileContent = file.readStringUntil('\n');
        break;
    }
    if (fileContent == "")
    {
        Serial.println("[\e[0;31mFAILED\e[0;37m] Reading file: empty");
    }
    else
    {
        Serial.println("[\e[0;32m  OK  \e[0;37m] Reading file: " + fileContent);
    }

    file.close();
    return fileContent;
}

void writeFile(const String path, const char *message)
{
    Serial.println(INDENT + "Writing file at " + path);
    File file = LittleFS.open(path, "w");
    if (!file)
    {
        Serial.println("[\e[0;31mFAILED\e[0;37m] Writing file");
        return;
    }
    Serial.println("[\e[0;32m  OK  \e[0;37m] Writing file");
    file.close();
}

bool loadCredentials()
{
    Serial.println(INDENT + "Loading credentials:");
    for (int i = 0; i < 5; i++)
    {
        creds[i] = readFile(paths[i]);
        if (creds[i] != "")
            Serial.println(paths[i] + " = " + creds[i]);
    }
    if (creds[_SSID] == "")
    {
        Serial.println("[\e[0;31mFAILED\e[0;37m] Loading credentials");
        return false;
    }
    else
    {
        Serial.println("[\e[0;32m  OK  \e[0;37m] Loading credentials");
        return true;
    }
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
    String contentType;
    if (path.endsWith("/"))
        path = "manager.html";
    if (path.endsWith(".html"))
        contentType = "text/html";
    if (path.endsWith(".css"))
        contentType = "text/css";
    if (LittleFS.exists(path))
    {
        AsyncWebServerResponse *response =
            request->beginResponse(LittleFS, path, contentType);
        request->send(response);
        return true;
    }
    return false;
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