#include <ArduinoOTA.h>
#include <ESP8266WebServer.h>
#include <LittleFS.h>

#include <PicoPrometheus.h>
#include <PicoUtils.h>
#include <PicoSyslog.h>

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
PicoUtils::Blink led_blinker(wifi_led, 0, 91);

PicoUtils::PinOutput<RELAY, false> relay;
FanControl fan_control(relay, settings, mqtt);

PicoUtils::PinInput<BUTTON, true> button;

PicoUtils::PinInput<SWITCH> toggle_switch;

PicoSyslog::SimpleLogger syslog("ventilatore");
Print & logger = syslog;

void setup_wifi() {
    WiFi.hostname(settings.net.hostname);
    WiFi.setAutoReconnect(true);

    syslog.println(F("Press button or toggle switch now to enter SmartConfig."));
    led_blinker.set_pattern(1);
    const PicoUtils::Stopwatch stopwatch;
    bool smart_config = false;
    {
        const bool initial_switch_pos = toggle_switch;
        while (!smart_config && (stopwatch.elapsed_millis() < 5 * 1000)) {
            smart_config = (button || toggle_switch != initial_switch_pos);
            delay(100);
        }
    }

    if (smart_config) {
        led_blinker.set_pattern(0b100100100 << 9);

        syslog.println(F("Entering SmartConfig mode."));
        WiFi.beginSmartConfig();
        while (!WiFi.smartConfigDone() && (stopwatch.elapsed_millis() < 5 * 60 * 1000)) {
            delay(100);
        }

        if (WiFi.smartConfigDone()) {
            syslog.println(F("SmartConfig success."));
        } else {
            syslog.println(F("SmartConfig failed.  Reboot."));
            ESP.reset();
        }
    } else {
        WiFi.softAPdisconnect(true);
        WiFi.begin();
    }

    led_blinker.set_pattern(0b10);
}

void setup() {
    wifi_led.init();

    led_blinker.set_pattern(0b10);
    PicoUtils::BackgroundBlinker bb(led_blinker);

    Serial.begin(115200);
    delay(5 * 1000);
    Serial.print(F("\n\n"
                   "Ventilatore " __DATE__ " " __TIME__
                   "\n\n"));

    setup_wifi();

    LittleFS.begin();

    {
        const auto config = PicoUtils::JsonConfigFile<StaticJsonDocument<1024>>(LittleFS, FPSTR(CONFIG_FILE));
        settings.load(config);
        settings.print();
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

    led_blinker.set_pattern(1);
}

PicoUtils::PeriodicRun switch_proc(0.25, [] {
    // TODO: Debounce?
    static bool last_switch_position = (bool) toggle_switch;
    bool switch_position = toggle_switch;
    if (switch_position != last_switch_position) {
        fan_control.mode = fan_control.is_fan_running() ? FanControl::OFF : FanControl::ON;
        syslog.printf("Entering force %s mode\n", fan_control.mode == FanControl::ON ? "ON" : "OFF");
    }
    last_switch_position = toggle_switch;
});

void update_status_led() {
    if (WiFi.status() == WL_CONNECTED) {
        if (mqtt.connected()) {
            led_blinker.set_pattern(uint64_t(0b101) << 60);
        } else {
            led_blinker.set_pattern(uint64_t(0b1) << 60);
        }
    } else {
        led_blinker.set_pattern(0b1100);
    }
    led_blinker.tick();
};

void loop() {
    ArduinoOTA.handle();  // this also handles MDNS updates

    server.handleClient();

    mqtt.loop();

    switch_proc.tick();
    fan_control.tick();

    update_status_led();
}
