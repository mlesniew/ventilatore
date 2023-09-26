#include <Arduino.h>
#include <PicoMQTT.h>
#include <PicoSyslog.h>

#include "fancontrol.h"
#include "hass.h"
#include "settings.h"

extern FanControl fan_control;
extern Settings settings;
extern PicoSyslog::SimpleLogger syslog;

namespace {

template <typename T>
class Watch {
    public:
        Watch(std::function<T()> getter, std::function<void(T)> callback):
            callback(callback), getter(getter), last_value(T(getter())) {
        }

        void tick() {
            T new_value = getter();
            if (new_value != last_value) {
                callback(new_value);
                last_value = new_value;
            }
        }

        std::function<void(T)> callback;

    protected:
        std::function<T()> getter;
        T last_value;
};

const String board_id(ESP.getChipId(), HEX);
PicoMQTT::Client mqtt;

void report_fan_state(bool running) {
    syslog.printf("Fan is now %s.\n", running ? "running" : "stopped");
    mqtt.publish("ventilatore/" + board_id + "/state", running ? "ON" : "OFF");
}

void report_fan_mode(FanControl::Mode mode) {
    const char * modestr = "auto";
    if (mode == FanControl::ON) { modestr = "on"; }
    if (mode == FanControl::OFF) { modestr = "off"; }

    syslog.printf("Mode set to %s.\n", modestr);
    mqtt.publish("ventilatore/" + board_id + "/mode", modestr);
}

Watch<bool> fan_state_watch(
    [] { return fan_control.is_fan_running(); },
    report_fan_state);

Watch<FanControl::Mode> fan_mode_watch(
    [] { return fan_control.mode; },
    report_fan_mode);

}

namespace HomeAssistant {

void autodiscovery() {
    syslog.println(F("Home Assistant autodiscovery messages..."));

    const String board_unique_id = "ventilatore-" + board_id;

    {
        const auto unique_id = board_unique_id + "-fan";
        const String topic = "homeassistant/fan/" + unique_id + "/config";

        StaticJsonDocument<1024> json;
        json["unique_id"] = unique_id;
        json["name"] = "Fan";
        json["availability_topic"] = "ventilatore/" + board_id + "/availability";
        json["state_topic"] = "ventilatore/" + board_id + "/state";
        json["command_topic"] = "ventilatore/" + board_id + "/state/set";
        json["preset_mode_state_topic"] = "ventilatore/" + board_id + "/mode";
        json["preset_mode_command_topic"] = "ventilatore/" + board_id + "/mode/set";
        json["preset_modes"][0] = "auto";
        json["preset_modes"][1] = "on";
        json["preset_modes"][2] = "off";

        auto device = json["device"];
        device["name"] = "Ventilatore";
        device["identifiers"][0] = unique_id;
        device["configuration_url"] = "http://" + WiFi.localIP().toString();
        device["sw_version"] = __DATE__ " " __TIME__;

        auto publish = mqtt.begin_publish(topic, measureJson(json), 0, true);
        serializeJson(json, publish);
        publish.send();
    }
}

void init() {
    mqtt.host = settings.hass.server;
    mqtt.port = settings.hass.port;
    mqtt.username = settings.hass.username;
    mqtt.password = settings.hass.password;
    mqtt.client_id = "ventilatore-" + board_id;

    mqtt.will.topic = "ventilatore/" + board_id + "/availability";
    mqtt.will.payload = "offline";
    mqtt.will.retain = true;

    mqtt.connected_callback = [] {
        syslog.println(F("Home Assistant MQTT connected."));

        // send autodiscovery messages
        autodiscovery();

        // notify about availability
        mqtt.publish(mqtt.will.topic, "online", 0, true);

        // notify about the current state
        report_fan_mode(fan_control.mode);
        report_fan_state(fan_control.is_fan_running());
    };

    auto handler = [](String payload) {
        payload.toLowerCase();
        if (payload == "on") {
            fan_control.mode = FanControl::ON;
        } else if (payload == "off") {
            fan_control.mode = FanControl::OFF;
        } else if (payload == "auto") {
            fan_control.mode = FanControl::AUTO;
        }
    };

    mqtt.subscribe("ventilatore/" + board_id + "/state/set", handler);
    mqtt.subscribe("ventilatore/" + board_id + "/mode/set", handler);

    mqtt.begin();
}

void loop() {
    mqtt.loop();
    fan_state_watch.tick();
    fan_mode_watch.tick();
}

}
