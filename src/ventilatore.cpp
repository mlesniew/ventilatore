#include <BME280I2C.h>
#include <ESP8266WebServer.h>
#include <FS.h>
// This include is not really needed, but PlatformIO fails to compile without it
#include <SPI.h>
#include <TM1637Display.h>
#include <WiFiManager.h>
#include <Wire.h>

#include <OneButton.h>

#include "stopwatch.h"
#include "led.h"

#define HOSTNAME "ventilatore"
#define RELAY D8
#define MEASUREMENT_INTERVAL 5000

enum display_state_t {
    DISPLAY_STATE_FIRST,
    HUMIDITY_1 = DISPLAY_STATE_FIRST,
    HUMIDITY_2,
    TEMPERATURE_1,
    TEMPERATURE_2,
    PRESSURE_1,
    PRESSURE_2,
    DISPLAY_STATE_LAST
} display_state = HUMIDITY_1;

BME280I2C sensors[] = {
    BME280I2C(BME280I2C::Settings(
                BME280::OSR_X1,
                BME280::OSR_X1,
                BME280::OSR_X1,
                BME280::Mode_Forced,
                BME280::StandbyTime_1000ms,
                BME280::Filter_Off,
                BME280::SpiEnable_False,
                0x76
                )),

    BME280I2C(BME280I2C::Settings(
                BME280::OSR_X1,
                BME280::OSR_X1,
                BME280::OSR_X1,
                BME280::Mode_Forced,
                BME280::StandbyTime_1000ms,
                BME280::Filter_Off,
                BME280::SpiEnable_False,
                0x77
                ))
};

ESP8266WebServer server;
TM1637Display display(D6 /* clk */, D7 /* dio */);
Stopwatch stopwatch;
BlinkingLed wifi_led(D4, 0, 250, true);
OneButton button(D0, false);

struct Measurements {
    float temp, hum, pres;
} measurements[2];

void update_readings() {
    printf("Reading sensors...\n");
    for (int i = 0; i < 2; ++i) {
        sensors[i].read(measurements[i].pres, measurements[i].temp, measurements[i].hum,
                        BME280::TempUnit_Celsius, BME280::PresUnit_hPa);
        printf("S%i    T:  %.1f *C    H: %.1f %%    P: %.1f hPa\n",
               i, measurements[i].temp, measurements[i].hum, measurements[i].pres);
    }
}

void update_display()
{
    switch (display_state) {
        case HUMIDITY_1:
            display.showText("H1");
            display.showNumberDec(measurements[0].hum, false, 2, 2);
            break;
        case TEMPERATURE_1:
            display.showText("t1");
            display.showNumberDec(measurements[0].temp, false, 2, 2);
            break;
        case PRESSURE_1:
            display.showText("P1");
            display.showNumberDec(measurements[0].pres / 100, false, 2, 2);
            break;
        case HUMIDITY_2:
            display.showText("H2");
            display.showNumberDec(measurements[1].hum, false, 2, 2);
            break;
        case TEMPERATURE_2:
            display.showText("t2");
            display.showNumberDec(measurements[1].temp, false, 2, 2);
            break;
        case PRESSURE_2:
            display.showText("P2");
            display.showNumberDec(measurements[1].pres / 100, false, 2, 2);
            break;
    }
}

void setup()
{
    Serial.begin(9600);
    display.clear();
    display.setBrightness(7);
    display.showText("init");

    SPIFFS.begin();

    Wire.begin();
    Wire.setClock(1000); // use slow speed mode

    for (unsigned int i = 0; i < 2; ++i) {
        while(!sensors[i].begin()) {
            printf("Could not find BME280 sensor #%i.\n", i);
            delay(1000);
        }
        switch(sensors[i].chipModel())
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
    }

    pinMode(RELAY, OUTPUT);
    digitalWrite(RELAY, LOW);

    WiFi.hostname(HOSTNAME);
    WiFi.begin("", "");
    WiFi.setAutoReconnect(true);

    button.attachClick([] {
        display_state = static_cast<display_state_t>(static_cast<int>(display_state) + 1);
        if (display_state == DISPLAY_STATE_LAST)
            display_state = DISPLAY_STATE_FIRST;
        update_display();
        });

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

    server.on("/status", []{
            const char pattern[] =
                "{\"fan_running\":%s,\"inside\":{\"temperature\":%.1f,\"pressure\":%.1f,\"humidity\":%.1f},\"outside\":{\"temperature\":%.1f,\"pressure\":%.1f,\"humidity\":%.1f}}";
            char buf[200];
            snprintf(buf, 200, pattern,
                    "true",
                    measurements[0].temp, measurements[0].hum, measurements[0].pres,
                    measurements[1].temp, measurements[1].hum, measurements[1].pres);
            server.send(200, "application/json", buf);
            });

    server.on("/metrics", []{
            const char pattern[] =
                "# HELP fan_status Fan status\n"
                "# TYPE fan_status gauge\n"
                "fan_status %i\n"
                "# HELP temperature Air temperature in degrees Celsius\n"
                "# TYPE temperature gauge\n"
                "temperature{sensor=\"inside\"} %.1f\n"
                "temperature{sensor=\"outside\"} %.1f\n"
                "# HELP pressure Air pressure in hectopascal\n"
                "# TYPE pressure gauge\n"
                "pressure{sensor=\"inside\"} %.1f\n"
                "pressure{sensor=\"outside\"} %.1f\n"
                "# HELP humidity Relative air humidity in percent\n"
                "# TYPE humidity gauge\n"
                "humidity{sensor=\"inside\"} %.1f\n"
                "humidity{sensor=\"outside\"} %.1f\n";
            char buf[800]; // pattern lenght is around 550
            snprintf(buf, 800, pattern,
                    1,
                    measurements[0].temp, measurements[1].temp,
                    measurements[0].pres, measurements[1].pres,
                    measurements[0].hum, measurements[1].hum);
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
        update_display();
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
    button.tick();

}
