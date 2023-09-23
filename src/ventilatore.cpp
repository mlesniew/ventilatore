#include <ArduinoOTA.h>
#include <ESP8266WebServer.h>
#include <LittleFS.h>

#include <PicoPrometheus.h>
#include <PicoUtils.h>
#include <WiFiManager.h>

#include "fancontrol.h"
#include "settings.h"

#define BLUE_LED 13 /* D7 */
#define RELAY 12 /* D6 */
#define SWITCH 4 /* D2 */
#define BUTTON 0 /* D3 */

const char CONFIG_FILE[] PROGMEM = "/config.json";

PicoUtils::RestfulServer<ESP8266WebServer> server;

Settings settings;

PicoMQTT::Client mqtt;

PicoPrometheus::Registry & get_prometheus() {
    static PicoPrometheus::Registry registry;
    return registry;
}

PicoUtils::PinOutput<BLUE_LED, true> wifi_led;
PicoUtils::WiFiControl<WiFiManager> wifi_control(wifi_led);

PicoUtils::PinOutput<RELAY, false> relay;
FanControl fan_control(relay, settings, mqtt);

PicoUtils::PinInput<BUTTON, true> button;
PicoUtils::ResetButton reset_button(button);

PicoUtils::PinInput<SWITCH> toggle_switch;

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
    wifi_control.init(settings.net.hostname.c_str(), "turbina0");

    LittleFS.begin();

    {
        const auto config = PicoUtils::JsonConfigFile<StaticJsonDocument<1024>>(LittleFS, FPSTR(CONFIG_FILE));
        settings.load(config);
        settings.print();
    }

    relay.init();
    fan_control.init();

    {
        get_prometheus().labels["module"] = "ventilatore";
        get_prometheus().register_metrics_endpoint(server);
    }

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

    server.on("/config.json", HTTP_GET, [] { server.sendJson(settings.get_json()); });

    server.on("/config.json", HTTP_POST, [] {
        StaticJsonDocument<1024> json;
        deserializeJson(json, server.arg("plain"));
        settings.load(json);
        settings.print();
        auto f = LittleFS.open(String(FPSTR(CONFIG_FILE)), "w");
        serializeJsonPretty(settings.get_json(), f);
        f.close();
        server.sendJson(settings.get_json());
    });

    server.begin();

    {
        mqtt.host = settings.mqtt.server;
        mqtt.port = settings.mqtt.port;
        mqtt.username = settings.mqtt.username;
        mqtt.password = settings.mqtt.password;
        mqtt.begin();
    }

    {
        ArduinoOTA.setHostname(settings.net.hostname.c_str());
        if (settings.net.ota_password.length()) {
            ArduinoOTA.setPassword(settings.net.ota_password.c_str());
        }
        ArduinoOTA.begin();
    }
}

PicoUtils::PeriodicRun switch_proc(0.25, [] {
    // TODO: Debounce?
    static bool last_switch_position = (bool) toggle_switch;
    bool switch_position = toggle_switch;
    if (switch_position != last_switch_position) {
        fan_control.mode = fan_control.is_fan_running() ? FanControl::OFF : FanControl::ON;
        Serial.printf("Entering force %s mode\n", fan_control.mode == FanControl::ON ? "ON" : "OFF");
    }
    last_switch_position = toggle_switch;
});

void loop() {
    ArduinoOTA.handle();

    server.handleClient();

    wifi_control.tick();

    mqtt.loop();

    switch_proc.tick();
    fan_control.tick();
}
