#include <ESP8266WebServer.h>
#include <FS.h>
// This include is not really needed, but PlatformIO fails to compile without it
#include <SPI.h>
#include <EnvironmentCalculations.h>
#include <OneButton.h>

#include "resetbutton.h"
#include "scrolling_display.h"
#include "sensor.h"
#include "settings.h"
#include "userinterface.h"
#include "util.h"

#include <utils/stopwatch.h>
#include <utils/led.h>
#include <utils/wifi_control.h>

#define HOSTNAME "ventilatore"
#define RELAY D8
#define SENSOR_RESET D5

ESP8266WebServer server;

Sensors sensors(SENSOR_RESET);
ScrollingDisplay display{D6 /* clk */, D7 /* dio */, 300};
Stopwatch stopwatch;
BlinkingLed wifi_led(D4, 0, 91, true);
OneButton button(D0, false);
FanControl fan_control(RELAY, sensors);
UserInterface ui(display, fan_control, sensors, button);
ResetButton reset_button(D0);

WiFiControl wifi_control(wifi_led);

float eslp(float pressure, float temperature) {
    return EnvironmentCalculations::EquivalentSeaLevelPressure(
            float(settings::settings.data.altitude),
            temperature,
            pressure);
}

void setup() {
    Serial.begin(9600);
    printf("\n\n");
    printf("                 _   _ _       _\n");
    printf("__   _____ _ __ | |_(_) | __ _| |_ ___  _ __ ___\n");
    printf("\\ \\ / / _ \\ '_ \\| __| | |/ _` | __/ _ \\| '__/ _ \\\n");
    printf(" \\ V /  __/ | | | |_| | | (_| | || (_) | | |  __/\n");
    printf("  \\_/ \\___|_| |_|\\__|_|_|\\__,_|\\__\\___/|_|  \\___|\n");
    printf("\n\n");

    reset_button.init();

    display.init();
    display.clear();
    display.set_text("init");

    settings::load();

    SPIFFS.begin();

    // enter WiFi setup mode only if button is pressed during start up
    const bool wifi_setup = digitalRead(D0) == HIGH;
    if (wifi_setup) {
        display.set_text("nEt SEtUP");
    }
    wifi_control.init(wifi_setup ? WiFiInitMode::setup : WiFiInitMode::saved,
                      HOSTNAME);

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

    server.on("/reset", []{
            server.send(200, "text/plain", "OK");
            reset();
            });

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
                    sensors.inside().temperature,
                    eslp(sensors.inside().pressure, sensors.inside().temperature),
                    sensors.inside().humidity,
                    sensors.outside().temperature,
                    eslp(sensors.outside().pressure, sensors.outside().temperature),
                    sensors.outside().humidity);
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
                "# HELP equivalent_sea_level_pressure Equivalent sea level pressure in hectopascal\n"
                "# TYPE equivalent_sea_level_pressure gauge\n"
                "equivalent_sea_level_pressure{sensor=\"inside\"} %.1f\n"
                "equivalent_sea_level_pressure{sensor=\"outside\"} %.1f\n"
                "# HELP humidity Relative air humidity in percent\n"
                "# TYPE humidity gauge\n"
                "humidity{sensor=\"inside\"} %.1f\n"
                "humidity{sensor=\"outside\"} %.1f\n"
                "# HELP humidity_difference_threshold Humidity difference at which the fan is switched\n"
                "# TYPE humidity_difference_threshold gauge\n"
                "humidity_difference_threshold{threshold=\"on\"} %.1f\n"
                "humidity_difference_threshold{threshold=\"off\"} %.1f\n";
            char buf[1000]; // pattern length is around 600
            snprintf(buf, 1000, pattern,
                    fan_control.fan_running() ? 1:0,
                    sensors.inside().temperature, sensors.outside().temperature,
                    sensors.inside().pressure, sensors.outside().pressure,
                    eslp(sensors.inside().pressure, sensors.inside().temperature),
                    eslp(sensors.outside().pressure, sensors.outside().temperature),
                    sensors.inside().humidity, sensors.outside().humidity,
                    double(settings::settings.data.auto_on_dh),
                    double(settings::settings.data.auto_off_dh));
            server.send(200, "text/plain", buf);
            });

    server.on("/config/load", []{
            const char pattern[] = "{\"autoOnDH\":%i,\"autoOffDH\":%i,\"checkInterval\":%i,\"altitude\":%i}";
            char buf[150];
            snprintf(buf, 150, pattern,
                    settings::settings.data.auto_on_dh, settings::settings.data.auto_off_dh,
                    settings::settings.data.sensor_check_interval,
                    settings::settings.data.altitude);
            server.send(200, "application/json", buf);
            });

    server.on("/config/save", []{
            auto auto_on_dh = server.arg("autoOnDH");
            auto auto_off_dh = server.arg("autoOffDH");
            auto interval = server.arg("checkInterval");
            auto altitude = server.arg("altitude");

            settings::settings.data.auto_on_dh = auto_on_dh.toInt();
            settings::settings.data.auto_off_dh = auto_off_dh.toInt();
            settings::settings.data.sensor_check_interval = interval.toInt();
            settings::settings.data.altitude = altitude.toInt();

            settings::sanitize();
            settings::print();
            settings::save();

            server.sendHeader("Location", "/config", true);
            server.send(302, "text/plain", "Config saved.");
            });

    server.serveStatic("/", SPIFFS, "/index.html");
    server.serveStatic("/config", SPIFFS, "/config.html");

    // this must go last
    server.serveStatic("/", SPIFFS, "/");

    server.begin();
}

void loop() {
    server.handleClient();

    if (stopwatch.elapsed_millis() / 1000 >= settings::settings.data.sensor_check_interval) {
        // it's time to read the data again
        if (!sensors.update()) {
            // Sensor read failed, reset to try to reconnect.  Reconnecting should also be
            // possible by just switching the SENSOR_RESET pin to low and to high again and
            // reinitializing the sensors.  However, this is a rare situation, so it's ok to
            // simply do a full reset.
            printf("Error reading sensors.\n");
            reset();
        }
        fan_control.tick();
        stopwatch.reset();
    }

    ui.tick();
    wifi_control.tick();
}
