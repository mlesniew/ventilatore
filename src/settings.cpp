#include <LittleFS.h>

#include <PicoUtils.h>

#include "settings.h"

extern Print & logger;

JsonDocument Settings::get_json() const {
    JsonDocument json;
    json["fan"]["humidity"] = fan.humidity;
    json["fan"]["hysteresis"] = fan.hysteresis;
    json["fan"]["force_timeout_minutes"] = fan.force_timeout_minutes;
    json["fan"]["max_auto_on_time"] = fan.max_auto_on_time;
    json["fan"]["min_auto_on_time"] = fan.min_auto_on_time;
    json["fan"]["max_auto_off_time"] = fan.max_auto_off_time;
    json["fan"]["min_auto_off_time"] = fan.min_auto_off_time;
    json["sensor"] = sensor;
    json["mqtt"]["server"] = mqtt.server;
    json["mqtt"]["port"] = mqtt.port;
    json["mqtt"]["username"] = mqtt.username;
    json["mqtt"]["password"] = mqtt.password;
    json["net"]["hostname"] = net.hostname;
    json["net"]["ota_password"] = net.ota_password;
    json["net"]["syslog"] = net.syslog;
    json["hass"]["server"] = hass.server;
    json["hass"]["port"] = hass.port;
    json["hass"]["username"] = hass.username;
    json["hass"]["password"] = hass.password;
    return json;
}

void Settings::load(const JsonDocument & json) {
    fan.humidity = json["fan"]["humidity"] | 10;
    fan.hysteresis = json["fan"]["hysteresis"] | 65;
    fan.force_timeout_minutes = json["fan"]["force_timeout_minutes"] | 15;
    fan.max_auto_on_time = json["fan"]["max_auto_on_time"] | 60;
    fan.min_auto_on_time = json["fan"]["min_auto_on_time"] | 20;
    fan.min_auto_off_time = json["fan"]["min_auto_off_time"] | 20;
    fan.min_auto_off_time = json["fan"]["max_auto_off_time"] | 720;
    sensor = json["sensor"] | "";
    mqtt.server = json["mqtt"]["server"] | "calor.local";
    mqtt.port = json["mqtt"]["port"] | 1883;
    mqtt.username = json["mqtt"]["username"] | "";
    mqtt.password = json["mqtt"]["password"] | "";
    net.hostname = json["net"]["hostname"] | "ventilatore";
    net.ota_password = json["net"]["ota_password"] | "";
    net.syslog = json["net"]["syslog"] | "";
    hass.server = json["hass"]["server"] | "";
    hass.port = json["hass"]["port"] | 1883;
    hass.username = json["hass"]["username"] | "";
    hass.password = json["hass"]["password"] | "";
    sanitize();
}

void Settings::sanitize() {
    if (fan.humidity > 100) {
        fan.humidity = 100;
    }

    if (fan.hysteresis > 50) {
        fan.hysteresis = 50;
    }

    sensor.trim();
}

void Settings::print() {
    serializeJsonPretty(get_json(), logger);
    logger.println();
}
