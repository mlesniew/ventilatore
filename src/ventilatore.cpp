#include <ArduinoOTA.h>
#include <ESP8266WebServer.h>
#include <LittleFS.h>

#include <PicoPrometheus.h>
#include <PicoUtils.h>
#include <PicoSyslog.h>

#include "fancontrol.h"
#include "hass.h"
#include "settings.h"

#define BLUE_LED 13 /* D7 */
#define RELAY 12 /* D6 */
#define SWITCH 4 /* D2 */
#define BUTTON 0 /* D3 */

const char CONFIG_FILE[] PROGMEM = "/config.json";

PicoUtils::RestfulServer<ESP8266WebServer> server;

Settings settings;

PicoMQ picomq;

PicoPrometheus::Registry & get_prometheus() {
    static PicoPrometheus::Registry registry;
    return registry;
}

PicoUtils::PinOutput wifi_led(BLUE_LED, true);
PicoUtils::WiFiControlSmartConfig wifi_control(wifi_led);

PicoUtils::PinOutput relay(RELAY);
FanControl fan_control(relay, settings, picomq);

PicoUtils::PinInput button(BUTTON, true);

PicoUtils::PinInput toggle_switch(SWITCH);

PicoSyslog::SimpleLogger syslog("ventilatore");
Print & logger = syslog;

void setup() {
    wifi_led.init();

    Serial.begin(115200);
    Serial.print(F("\n\n"
                   "Ventilatore " __DATE__ " " __TIME__
                   "\n\n"));

    LittleFS.begin();

    {
        const auto config = PicoUtils::JsonConfigFile<JsonDocument>(LittleFS, FPSTR(CONFIG_FILE));
        settings.load(config);
        settings.print();
    }

    {
        WiFi.hostname(settings.net.hostname);
        struct XORInput: PicoUtils::BinaryInput {
            XORInput(PicoUtils::BinaryInput & a, PicoUtils::BinaryInput & b): a(a), b(b) {}
            PicoUtils::BinaryInput & a;
            PicoUtils::BinaryInput & b;
            bool get() const override { return a.get() != b.get(); }
        } xor_input(button, toggle_switch);
        wifi_control.init(xor_input, true);
        wifi_control.get_connectivity_level = [] {
            return 1 + (fan_control.healthcheck() ? 1 : 0) + (HomeAssistant::connected() ? 1 : 0);
        };
    }

    syslog.server = settings.net.syslog;

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
        JsonDocument json;
        deserializeJson(json, server.arg("plain"));
        settings.load(json);
        settings.print();
        auto f = LittleFS.open(String(FPSTR(CONFIG_FILE)), "w");
        serializeJsonPretty(settings.get_json(), f);
        f.close();
        server.sendJson(settings.get_json());
    });

    server.begin();

    picomq.begin();

    {
        ArduinoOTA.setHostname(settings.net.hostname.c_str());
        if (settings.net.ota_password.length()) {
            ArduinoOTA.setPassword(settings.net.ota_password.c_str());
        }
        ArduinoOTA.begin();
    }

    HomeAssistant::init();
}

PicoUtils::PeriodicRun switch_proc(0.25, [] {
    // TODO: Debounce?
    static bool last_switch_position = (bool) toggle_switch;
    bool switch_position = toggle_switch;
    if (switch_position != last_switch_position) {
        fan_control.mode = fan_control.is_fan_running() ? FanControl::OFF : FanControl::ON;
    }
    last_switch_position = toggle_switch;
});

void loop() {
    ArduinoOTA.handle();  // this also handles MDNS updates

    server.handleClient();

    picomq.loop();

    switch_proc.tick();
    fan_control.tick();

    wifi_control.tick();

    HomeAssistant::loop();
}
