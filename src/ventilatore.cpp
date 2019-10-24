#include <BME280I2C.h>
#include <ESP8266WebServer.h>
#include <FS.h>
// This include is not really needed, but PlatformIO fails to compile without it
#include <SPI.h>
#include <TM1637Display.h>
#include <WiFiManager.h>
#include <Wire.h>

#include "stopwatch.h"
#include "led.h"

#define HOSTNAME "ventilatore"
#define RELAY D8
#define MEASUREMENT_INTERVAL 5000

BME280I2C bme280;
ESP8266WebServer server;
TM1637Display display(D6 /* clk */, D5 /* dio */);
Stopwatch stopwatch;
BlinkingLed wifi_led(D4, 0, 250, true);

struct Measurements {
    float temp, hum, pres;
} measurements;

void update_readings() {
    bme280.read(measurements.pres, measurements.temp, measurements.hum,
                BME280::TempUnit_Celsius, BME280::PresUnit_hPa);
    printf("T:  %.1f *C    H: %.1f %%    P: %.1f hPa\n",
           measurements.temp, measurements.hum, measurements.pres);
}

void setup()
{
    Serial.begin(9600);
    display.clear();
    display.setBrightness(7);
    display.showText("init");

    SPIFFS.begin();

    Wire.begin();
    Wire.setClock(10000); // use slow speed mode

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
    WiFi.begin("", "");
    WiFi.setAutoReconnect(true);

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
            server.send(200, "text/plain", String(measurements.temp));
            });

    server.on("/humidity", []{
            server.send(200, "text/plain", String(measurements.hum));
            });

    server.on("/pressure", []{
            server.send(200, "text/plain", String(measurements.pres));
            });

    server.on("/readings", []{
            char buf[80];
            const char pattern[] = "{"
                "\"temperature\":%.1f,"
                "\"humidity\":%.1f,"
                "\"pressure\":%.1f"
                "}";
            snprintf(buf, 80, pattern,
                     measurements.temp, measurements.hum, measurements.pres);
            server.send(200, "text/plain", buf);
            });

    server.begin();

    display.showText("LOOP");
}

void loop()
{
    server.handleClient();

    if (stopwatch.elapsedMillis() >= MEASUREMENT_INTERVAL) {
        // it's time to read the data again
        update_readings();
        display.showText("H");
        display.showNumberDec(measurements.hum, false, 3, 1);
        stopwatch.reset();
    }

    static wl_status_t previous_wifi_status = WL_NO_SHIELD;
    wl_status_t current_wifi_status = WiFi.status();

    if (current_wifi_status != previous_wifi_status) {
        switch (current_wifi_status)
        {
            case WL_CONNECTED:
                wifi_led.set_pattern(1);
                break;
            case WL_DISCONNECTED:
                wifi_led.set_pattern(0);
                break;
            default:
                wifi_led.set_pattern(0b10);
                break;
        }
        printf("WiFi: %i -> %i\n", (int)(previous_wifi_status), (int)(current_wifi_status));
        previous_wifi_status = current_wifi_status;
    }

    wifi_led.tick();

}
