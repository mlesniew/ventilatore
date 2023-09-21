#include <LittleFS.h>

#include <PicoUtils.h>

#include "settings.h"

#define DEFAULT_AUTO_ON_DH 15
#define DEFAULT_AUTO_OFF_DH 20

const char CONFIG_FILE[] PROGMEM = "/config.json";

DynamicJsonDocument Settings::get_json() const {
    DynamicJsonDocument json(256);
    json["thresholds"]["on"] = auto_on_dh;
    json["thresholds"]["off"] = auto_off_dh;
    json["sensors"]["inside"] = inside_sensor_address;
    json["sensors"]["outside"] = outside_sensor_address;
    json["force_timeout_minutes"] = force_timeout_minutes;
    return json;
}

void Settings::load(const JsonDocument & json) {
    auto_on_dh = json["thresholds"]["on"] | DEFAULT_AUTO_ON_DH;
    auto_off_dh = json["thresholds"]["off"] | DEFAULT_AUTO_OFF_DH;
    inside_sensor_address = json["sensors"]["inside"] | "";
    outside_sensor_address = json["sensors"]["outside"] | "";
    force_timeout_minutes = json["force_timeout_minutes"] | 15;
    sanitize();
}

void Settings::sanitize() {
    if (auto_on_dh > 100) {
        auto_on_dh = 100;
    }

    if (auto_off_dh > 100) {
        auto_off_dh = 100;
    }

    // auto_on_dh must be grater or equal to auto_off_dh
    if (auto_off_dh > auto_on_dh) {
        const auto tmp = auto_off_dh;
        auto_off_dh = auto_on_dh;
        auto_on_dh = tmp;
    }

    inside_sensor_address.toLowerCase();
    inside_sensor_address.trim();
    outside_sensor_address.toLowerCase();
    outside_sensor_address.trim();
}

void Settings::load() {
    const auto config = PicoUtils::JsonConfigFile<StaticJsonDocument<256>>(LittleFS, FPSTR(CONFIG_FILE));
    load(config);
    print();
}

void Settings::save() const {
    // TODO
}

void Settings::print() {
    serializeJsonPretty(get_json(), Serial);
}
