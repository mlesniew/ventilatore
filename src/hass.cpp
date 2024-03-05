#include <Arduino.h>
#include <PicoMQTT.h>
#include <PicoSyslog.h>
#include <PicoUtils.h>

#include "fancontrol.h"
#include "hass.h"
#include "settings.h"

extern FanControl fan_control;
extern Settings settings;
extern PicoSyslog::SimpleLogger syslog;

namespace {

const String board_id(ESP.getChipId(), HEX);
const String topic_prefix = "ventilatore/" + board_id + "/";

PicoMQTT::Client mqtt;

void report_fan_state(bool running) {
    syslog.printf("Fan is now %s.\n", running ? "running" : "stopped");
    mqtt.publish(topic_prefix + "state", running ? "ON" : "OFF", 0, true);
}

void report_fan_mode(FanControl::Mode mode) {
    const char * modestr = "auto";
    if (mode == FanControl::ON) { modestr = "on"; }
    if (mode == FanControl::OFF) { modestr = "off"; }

    syslog.printf("Mode set to %s.\n", modestr);
    mqtt.publish(topic_prefix + "mode", modestr, 0, true);
}

void report_target_humidity(double value) {
    syslog.printf("Target humidity is now %f.\n", value);
    mqtt.publish(topic_prefix + "target_humidity", String(value), 0, true);
}

void report_hysteresis(double value) {
    syslog.printf("Humidity hysteresis is now %f.\n", value);
    mqtt.publish(topic_prefix + "humidity_hysteresis", String(value), 0, true);
}

std::list<PicoUtils::WatchInterface *> watches;

}

namespace HomeAssistant {

void autodiscovery() {
    syslog.println(F("Home Assistant autodiscovery messages..."));

    const String board_unique_id = "ventilatore-" + board_id;

    {
        const String unique_id = board_unique_id + "-fan";
        const String topic = "homeassistant/fan/" + unique_id + "/config";

        JsonDocument json;
        json["unique_id"] = unique_id;
        json["name"] = "Fan";
        json["availability_topic"] = topic_prefix + "availability";
        json["state_topic"] = topic_prefix + "state";
        json["command_topic"] = topic_prefix + "state/set";
        json["preset_mode_state_topic"] = topic_prefix + "mode";
        json["preset_mode_command_topic"] = topic_prefix + "mode/set";
        json["preset_modes"][0] = "auto";
        json["preset_modes"][1] = "on";
        json["preset_modes"][2] = "off";

        auto device = json["device"];
        device["name"] = "Ventilatore";
        device["identifiers"][0] = board_unique_id;
        device["configuration_url"] = "http://" + WiFi.localIP().toString();
        device["sw_version"] = __DATE__ " " __TIME__;

        auto publish = mqtt.begin_publish(topic, measureJson(json), 0, true);
        serializeJson(json, publish);
        publish.send();
    }

    struct ConfigNumber {
        const char * unique_id;
        const char * name;
        const char * topic;
        unsigned int min;
        unsigned int max;
    };

    static const ConfigNumber config_numbers[] = {
        {"target-humidity", "Target humidity", "target_humidity", 20, 80},
        {"humidity-hysteresis", "Humidity hysteresis", "humidity_hysteresis", 5, 20},
    };

    for (const auto & e : config_numbers) {
        const auto unique_id = board_unique_id + "-" + e.unique_id;
        const String topic = "homeassistant/number/" + unique_id + "/config";
        JsonDocument json;
        json["unique_id"] = unique_id;
        json["name"] = e.name;
        json["availability_topic"] = topic_prefix + "availability";
        json["state_topic"] = topic_prefix + e.topic;
        json["command_topic"] = topic_prefix + e.topic + "/set";
        json["retain"] = true;
        json["unit_of_measurement"] = "%";
        json["min"] = e.min;
        json["max"] = e.max;
        json["step"] = 1;
        json["device_class"] = "humidity";
        json["entity_category"] = "config";

        auto device = json["device"];
        device["name"] = "Ventilatore";
        device["identifiers"][0] = board_unique_id;
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
        for (auto & watch : watches) { watch->fire(); }
    };

    watches.push_back(new PicoUtils::Watch<bool>([] { return fan_control.is_fan_running(); }, report_fan_state));
    watches.push_back(new PicoUtils::Watch<FanControl::Mode>([] { return fan_control.mode; }, report_fan_mode));
    watches.push_back(new PicoUtils::Watch<double>([] { return settings.fan.humidity; }, report_target_humidity));
    watches.push_back(new PicoUtils::Watch<double>([] { return settings.fan.hysteresis; }, report_hysteresis));

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

    mqtt.subscribe("ventilatore/" + board_id + "/target_humidity/set", [](String payload) {
        settings.fan.humidity = payload.toDouble();
        settings.sanitize();
    });
    mqtt.subscribe("ventilatore/" + board_id + "/humidity_hysteresis/set", [](String payload) {
        settings.fan.hysteresis = payload.toDouble();
        settings.sanitize();
    });

    mqtt.begin();
}

void loop() {
    mqtt.loop();
    for (auto & watch : watches) { watch->tick(); }
}

}
