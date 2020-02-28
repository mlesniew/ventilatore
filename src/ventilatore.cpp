#include <ESP8266WebServer.h>
#include <FS.h>
// This include is not really needed, but PlatformIO fails to compile without it
#include <SPI.h>

#include <OneButton.h>

#include "fancontrol.h"
#include "led.h"
#include "scrolling_display.h"
#include "sensor.h"
#include "stopwatch.h"
#include "userinterface.h"
#include "wificontrol.h"

#define HOSTNAME "ventilatore"
#define RELAY D8
#define MEASUREMENT_INTERVAL 5000

ESP8266WebServer server;

Sensors sensors;
ScrollingDisplay display{D6 /* clk */, D7 /* dio */, 300};
Stopwatch stopwatch;
BlinkingLed wifi_led(D4, 0, 91, true);
OneButton button(D0, false);
FanControl fan_control(RELAY, sensors);
UserInterface ui(display, fan_control, sensors, button);

WiFiControl wifi_control(HOSTNAME, wifi_led);

void reset() {
    printf("Reset...\n");
    while (1) {
        ESP.restart();
        delay(1000);
    }
}

void setup() {
    display.init();
    display.clear();
    Serial.begin(9600);

    SPIFFS.begin();

    // enter WiFi setup mode only if button is pressed during start up
    bool wifi_setup = digitalRead(D0) == HIGH;
    wifi_control.init(wifi_setup);

    if (!sensors.init()) {
        printf("Failed to initialize sensors.\n");
        display.set_next_text("SEnSor Error");
        delay(120 * 1000);
        reset();
    }

    fan_control.init();

    server.on("/version", []{
            server.send(200, "text/plain", __DATE__ " " __TIME__);
            });

    server.on("/uptime", []{
            server.send(200, "text/plain", String(millis() / 1000));
            });

    server.serveStatic("/", SPIFFS, "/index.html");

    server.on("/on", []{
            fan_control.force_on();
            server.send(200, "text/plain", "OK");
            });

    server.on("/off", []{
            fan_control.force_off();
            server.send(200, "text/plain", "OK");
            });

    server.on("/status", []{
            const char pattern[] =
                "{\"fan_running\":%s,\"inside\":{\"temperature\":%.1f,\"pressure\":%.1f,\"humidity\":%.1f},\"outside\":{\"temperature\":%.1f,\"pressure\":%.1f,\"humidity\":%.1f}}";
            char buf[200];
            snprintf(buf, 200, pattern,
                    fan_control.fan_running() ? "true" : "false",
                    sensors.inside().temperature, sensors.inside().pressure, sensors.inside().humidity,
                    sensors.outside().temperature, sensors.outside().pressure, sensors.outside().humidity);
            server.send(200, "application/json", buf);
            });

    server.on("/metrics", []{
            const char pattern[] =
                "# HELP fan_running Fan status\n"
                "# TYPE fan_running gauge\n"
                "fan_running %i\n"
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
                    fan_control.fan_running() ? 1:0,
                    sensors.inside().temperature, sensors.outside().temperature,
                    sensors.inside().pressure, sensors.outside().pressure,
                    sensors.inside().humidity, sensors.outside().humidity);
            server.send(200, "text/plain", buf);
            });

    server.begin();
}

void loop() {
    server.handleClient();


    if (stopwatch.elapsedMillis() >= MEASUREMENT_INTERVAL) {
        // it's time to read the data again
        sensors.update();
        fan_control.tick();
        stopwatch.reset();
    }

    ui.tick();
    wifi_control.tick();
}
