#include "webserver.hpp"
#include "filesystem.hpp"
#include "ledmatrix.hpp"

AsyncWebServer webServer(80); // webserver at Port 80
IPAddress localIP;
IPAddress localGateway;
IPAddress localSubnet;
DNSServer dnsServer;

/* Init control vars */
uint8_t dieRollResult = 0;
unsigned long lastActionTime = 0;
bool rollRequested = false;
bool sleepRequested = false;
bool managerRequested = false;

uint8_t diceRed = 120;
uint8_t diceGreen = 120;
uint8_t diceBlue = 120;

void dnsNext() {
    // we only handle dns requests while the wifi manager is running
    dnsServer.processNextRequest();
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
    Serial.println(INDENT + "Connect to: " + AP_SSID + " with password: " + AP_PW);
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
    hostIndex(); // host the usual webpage
    Serial.println();
    Serial.println("[\e[0;32m  OK  \e[0;37m] Connecting to WiFi");
    return true;
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

    webServer.begin(); // start ws
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
            Serial.println(INDENT + response);
            request->send(200, "text/plain", response);
            printPattern(BIGDOT, 0, 255, 255);
            delay(150);
            printPattern(0);
            ESP.restart(); });
    webServer.begin();
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