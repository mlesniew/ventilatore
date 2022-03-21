#include <memory>
#include <ESP8266WebServer.h>
#include <LittleFS.h>
// This include is not really needed, but PlatformIO fails to compile without it
#include <SPI.h>
#include <EnvironmentCalculations.h>
#include <OneButton.h>

#include "scrolling_display.h"
#include "sensor.h"
#include "settings.h"
#include "userinterface.h"

#include <utils/led.h>
#include <utils/reset.h>
#include <utils/reset_button.h>
#include <utils/stopwatch.h>
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
ResetButton reset_button(D0, HIGH, 20);

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

    LittleFS.begin();

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
                "# HELP air_temperature Air temperature in degrees Celsius\n"
                "# TYPE air_temperature gauge\n"
                "air_temperature{name=\"%s\", sensor=\"inside\"} %.1f\n"
                "air_temperature{name=\"%s\", sensor=\"outside\"} %.1f\n"
                "# HELP atmospheric_pressure Atmospheric pressure (equivalent sea level) in hectopascal\n"
                "# TYPE atmospheric_pressure gauge\n"
                "atmospheric_pressure{name=\"%s\", sensor=\"inside\"} %.1f\n"
                "atmospheric_pressure{name=\"%s\", sensor=\"outside\"} %.1f\n"
                "# HELP humidity Relative air humidity in percent\n"
                "# TYPE humidity gauge\n"
                "air_humidity{name=\"%s\", sensor=\"inside\"} %.1f\n"
                "air_humidity{name=\"%s\", sensor=\"outside\"} %.1f\n"
                "# HELP humidity_difference_threshold Humidity difference at which the fan is switched\n"
                "# TYPE humidity_difference_threshold gauge\n"
                "humidity_difference_threshold{threshold=\"on\"} %.1f\n"
                "humidity_difference_threshold{threshold=\"off\"} %.1f\n";
            const char * inside_sensor_name = settings::settings.data.inside_sensor_name;
            const char * outside_sensor_name = settings::settings.data.outside_sensor_name;

            // ESP stack is only 4K, so we must use the heap here
            std::unique_ptr<char[]> buf{new char[1500]};
            snprintf(buf.get(), 1500, pattern,
                    fan_control.fan_running() ? 1:0,
                    inside_sensor_name, sensors.inside().temperature,
                    outside_sensor_name, sensors.outside().temperature,
                    inside_sensor_name, eslp(sensors.inside().pressure, sensors.inside().temperature),
                    outside_sensor_name, eslp(sensors.outside().pressure, sensors.outside().temperature),
                    inside_sensor_name, sensors.inside().humidity,
                    outside_sensor_name, sensors.outside().humidity,
                    double(settings::settings.data.auto_on_dh),
                    double(settings::settings.data.auto_off_dh));
            server.send(200, "text/plain", buf.get());
            });

    server.on("/config/load", []{
            const char pattern[] = "{\"autoOnDH\":%i,\"autoOffDH\":%i,\"checkInterval\":%i,\"altitude\":%i, "
                    "\"inside_sensor_name\": \"%s\", \"outside_sensor_name\": \"%s\"}";
            char buf[300];
            snprintf(buf, 300, pattern,
                    settings::settings.data.auto_on_dh,
                    settings::settings.data.auto_off_dh,
                    settings::settings.data.sensor_check_interval,
                    settings::settings.data.altitude,
                    settings::settings.data.inside_sensor_name,
                    settings::settings.data.outside_sensor_name);
            server.send(200, "application/json", buf);
            });

    server.on("/config/save", []{
            auto auto_on_dh = server.arg("autoOnDH");
            auto auto_off_dh = server.arg("autoOffDH");
            auto interval = server.arg("checkInterval");
            auto altitude = server.arg("altitude");
            auto inside_sensor_name = server.arg("inside_sensor_name");
            auto outside_sensor_name = server.arg("outside_sensor_name");

            settings::settings.data.auto_on_dh = auto_on_dh.toInt();
            settings::settings.data.auto_off_dh = auto_off_dh.toInt();
            settings::settings.data.sensor_check_interval = interval.toInt();
            settings::settings.data.altitude = altitude.toInt();

            inside_sensor_name.toCharArray(
                    settings::settings.data.inside_sensor_name,
                    settings::SENSOR_NAME_MAX_LENGTH);

            outside_sensor_name.toCharArray(
                    settings::settings.data.outside_sensor_name,
                    settings::SENSOR_NAME_MAX_LENGTH);

            settings::sanitize();
            settings::print();
            settings::save();

            server.sendHeader("Location", "/config", true);
            server.send(302, "text/plain", "Config saved.");
            });

    server.serveStatic("/", LittleFS, "/index.html");
    server.serveStatic("/config", LittleFS, "/config.html");

    // this must go last
    server.serveStatic("/", LittleFS, "/");

    server.begin();
}

void loop() {
    server.handleClient();

    if (stopwatch.elapsed() >= settings::settings.data.sensor_check_interval) {
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
