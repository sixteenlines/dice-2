#include "filesystem.hpp"

/* Credentials in RAM */
String creds[5] = {
    "", // SSID
    "", // Password
    "", // IP
    "", // Gateway
    ""  // Subnet
};

JSONVar settings;

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

    deep_sleep = readFile(paths[_DEVICE_TO]).toInt();
    led_sleep = readFile(paths[_LED_TO]).toInt();
    if (deep_sleep != 0 && led_sleep != 0)
    {
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
        Serial.println("[\e[0;31mFAILED\e[0;37m] Opening file");
        return;
    }
    else if (file.print(message))
    {
        Serial.println("[\e[0;32m  OK  \e[0;37m] Writing file");
    }
    else
    {
        Serial.println("[\e[0;31mFAILED\e[0;37m] Reading file");
    }
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