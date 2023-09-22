#include <LittleFS.h>

#include <PicoUtils.h>

#include "settings.h"

DynamicJsonDocument Settings::get_json() const {
    DynamicJsonDocument json(256);
    json["fan"]["auto_on_humidity_difference"] = fan.auto_on_dh;
    json["fan"]["auto_off_humidity_difference"] = fan.auto_off_dh;
    json["fan"]["force_timeout_minutes"] = fan.force_timeout_minutes;
    json["sensors"]["inside"] = sensors.inside;
    json["sensors"]["outside"] = sensors.outside;
    json["mqtt"]["server"] = mqtt.server;
    json["mqtt"]["port"] = mqtt.port;
    json["mqtt"]["username"] = mqtt.username;
    json["mqtt"]["password"] = mqtt.password;
    json["net"]["hostname"] = net.hostname;
    json["net"]["ota_password"] = net.ota_password;
    return json;
}

void Settings::load(const JsonDocument & json) {
    fan.auto_on_dh = json["fan"]["auto_on_humidity_difference"] | 10;
    fan.auto_off_dh = json["fan"]["auto_off_humidity_difference"] | 5;
    fan.force_timeout_minutes = json["fan"]["force_timeout_minutes"] | 15;
    sensors.inside = json["sensors"]["inside"] | "";
    sensors.outside = json["sensors"]["outside"] | "";
    mqtt.server = json["mqtt"]["server"] | "calor.local";
    mqtt.port = json["mqtt"]["port"] | 1883;
    mqtt.username = json["mqtt"]["username"] | "";
    mqtt.password = json["mqtt"]["password"] | "";
    net.hostname = json["net"]["hostname"] | "ventilatore";
    net.ota_password = json["net"]["ota_password"] | "";
    sanitize();
}

void Settings::sanitize() {
    if (fan.auto_on_dh > 100) {
        fan.auto_on_dh = 100;
    }

    if (fan.auto_off_dh > 100) {
        fan.auto_off_dh = 100;
    }

    // auto_on_dh must be grater or equal to auto_off_dh
    if (fan.auto_off_dh > fan.auto_on_dh) {
        const auto tmp = fan.auto_off_dh;
        fan.auto_off_dh = fan.auto_on_dh;
        fan.auto_on_dh = tmp;
    }

    sensors.inside.toLowerCase();
    sensors.inside.trim();
    sensors.outside.toLowerCase();
    sensors.outside.trim();
}

void Settings::print() {
    serializeJsonPretty(get_json(), Serial);
    Serial.println();
}
