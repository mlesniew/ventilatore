#include <ESP8266WebServer.h>
#include <LittleFS.h>

#include <WiFiManager.h>
#include <PicoUtils.h>

#include "fancontrol.h"
#include "settings.h"

#define HOSTNAME "ventilatore"
#define PASSWORD "turbina0"

#define RELAY D8

PicoUtils::RestfulServer<ESP8266WebServer> server;

Settings settings;

PicoMQTT::Client mqtt;

PicoUtils::PinOutput<D4, true> wifi_led;
PicoUtils::WiFiControl<WiFiManager> wifi_control(wifi_led);

PicoUtils::PinOutput<RELAY, false> relay;
FanControl fan_control(relay, settings, mqtt);

PicoUtils::PinInput<D0, true> button;
PicoUtils::ResetButton reset_button(button);

void setup() {
    wifi_led.init();
    wifi_led.set(true);

    Serial.begin(115200);

    printf("\n\n");
    printf("                 _   _ _       _\n");
    printf("__   _____ _ __ | |_(_) | __ _| |_ ___  _ __ ___\n");
    printf("\\ \\ / / _ \\ '_ \\| __| | |/ _` | __/ _ \\| '__/ _ \\\n");
    printf(" \\ V /  __/ | | | |_| | | (_| | || (_) | | |  __/\n");
    printf("  \\_/ \\___|_| |_|\\__|_|_|\\__,_|\\__\\___/|_|  \\___|\n");
    printf("\n\n");

    // reset_button.init();
    wifi_control.init(HOSTNAME, PASSWORD);

    LittleFS.begin();

    settings.load();

    relay.init();
    fan_control.init();

    server.on("/reset", [] {
        server.send(200);
        ESP.reset();
    });

    server.on("/on", [] {
        fan_control.mode = FanControl::ON;
        server.send(200);
    });

    server.on("/off", [] {
        fan_control.mode = FanControl::OFF;
        server.send(200);
    });

    server.on("/status", [] {
        server.sendJson(fan_control.get_json());
    });

    server.on("/config/load", [] { server.sendJson(settings.get_json()); });

    server.on("/config/save", [] {
        auto auto_on_dh = server.arg("autoOnDH");
        auto auto_off_dh = server.arg("autoOffDH");

        settings.auto_on_dh = auto_on_dh.toInt();
        settings.auto_off_dh = auto_off_dh.toInt();

        settings.sanitize();
        settings.print();
        settings.save();

        server.sendHeader("Location", "/config", true);
        server.send(302, "text/plain", "Config saved.");
    });

    server.begin();

    mqtt.host = "calor.local";
    mqtt.begin();
}

void loop() {
    server.handleClient();

    wifi_control.tick();

    mqtt.loop();

    // it's time to read the data again
    fan_control.tick();
}
