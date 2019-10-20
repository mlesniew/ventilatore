#include <BME280I2C.h>
#include <ESP8266WebServer.h>
#include <FS.h>
// This include is not really needed, but PlatformIO fails to compile without it
#include <SPI.h>
#include <TM1637Display.h>
#include <WiFiManager.h>
#include <Wire.h>

#define HOSTNAME "ventilatore"
#define RELAY D8

BME280I2C bme280;
ESP8266WebServer server;
TM1637Display display(D6 /* clk */, D5 /* dio */);

void setup()
{
    Serial.begin(9600);
    display.clear();
    display.setBrightness(7);
    display.showText("bOOt");

    SPIFFS.begin();

    Wire.begin();
    while(!bme280.begin())
    {
        Serial.println("Could not find BME280 sensor!");
        delay(1000);
    }

    switch(bme280.chipModel())
    {
        case BME280::ChipModel_BME280:
            Serial.println("Found BME280 sensor! Success.");
            break;
        case BME280::ChipModel_BMP280:
            Serial.println("Found BMP280 sensor! No Humidity available.");
            break;
        default:
            Serial.println("Found UNKNOWN sensor! Error!");
    }

    pinMode(RELAY, OUTPUT);
    digitalWrite(RELAY, LOW);

    WiFi.hostname(HOSTNAME);

    WiFiManager wifiManager;
    wifiManager.autoConnect(HOSTNAME, "secret");

    server.on("/version", []{
            server.send(200, "text/plain", __DATE__ " " __TIME__);
            });

    server.on("/uptime", []{
            server.send(200, "text/plain", String(millis() / 1000));
            });

    server.serveStatic("/", SPIFFS, "/index.html");

    server.on("/on", []{
            digitalWrite(RELAY, HIGH);
            display.showText(" on ");
            server.send(200, "text/plain", "OK");
            });

    server.on("/off", []{
            digitalWrite(RELAY, LOW);
            display.showText(" oFF");
            server.send(200, "text/plain", "OK");
            });

    server.on("/temperature", []{
            const auto reading = bme280.temp();
            server.send(200, "text/plain", String(reading));
            });

    server.on("/humidity", []{
            const auto reading = bme280.hum();
            server.send(200, "text/plain", String(reading));
            });

    server.on("/pressure", []{
            const auto reading = bme280.pres(BME280::PresUnit_hPa);
            server.send(200, "text/plain", String(reading));
            });

    server.on("/readings", []{
            char buf[80];
            const char pattern[] = "{"
                "\"temperature\":%.1f,"
                "\"humidity\":%.1f,"
                "\"pressure\":%.1f"
                "}";
            float temp(NAN), hum(NAN), pres(NAN);
            bme280.read(pres, temp, hum,
                        BME280::TempUnit_Celsius, BME280::PresUnit_hPa);
            snprintf(buf, 80, pattern, temp, hum, pres);
            server.send(200, "text/plain", buf);
            });

    server.begin();

    display.showText("LOOP");
}

void loop()
{
    server.handleClient();
}
