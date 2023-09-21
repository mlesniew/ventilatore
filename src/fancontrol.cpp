#include <limits>

#include "Arduino.h"

#include "fancontrol.h"
#include "settings.h"

FanControl::FanControl(PicoUtils::BinaryOutput & relay, const Settings & settings, PicoMQTT::Client & mqtt)
    : mode(OFF), fan_running(false), relay(relay), settings(settings), mqtt(mqtt),
      humidity_inside(std::numeric_limits<double>::quiet_NaN()),
      humidity_outside(std::numeric_limits<double>::quiet_NaN()) {
}

DynamicJsonDocument FanControl::get_json() const {
    StaticJsonDocument<512> json;
    json["fan_running"] = fan_running;
    json["inside"]["humidity"] = (double) humidity_inside;
    json["outside"]["humidity"] = (double) humidity_outside;
    switch (mode) {
        case ON: json["mode"] = "on"; break;
        case OFF: json["mode"] = "off"; break;
        case AUTO: json["mode"] = "auto"; break;
    }
    return json;
}

void FanControl::init() {
    const auto handler = [this](Stream & stream, const char * name, PicoUtils::TimedValue<double> & value) {
        StaticJsonDocument<512> json;
        if (deserializeJson(json, stream) || !json.containsKey("humidity")) {
            return;
        }
        value = json["humidity"].as<double>();
        Serial.printf("Updated %s humidity to %.1f %%; current humidity difference is %.1f %%\n",
                      name, (double) value, (double) humidity_inside - (double) humidity_outside);
    };

    if (settings.inside_sensor_address.length()) {
        mqtt.subscribe("celsius/+/" + settings.inside_sensor_address, [handler, this](const char *, Stream & stream) {
            handler(stream, "inside", humidity_inside);
        });
    }

    if (settings.outside_sensor_address.length()) {
        mqtt.subscribe("celsius/+/" + settings.outside_sensor_address, [handler, this](const char *, Stream & stream) {
            handler(stream, "outside", humidity_outside);
        });
    }

    mode = AUTO;
}

void FanControl::tick() {
    const auto deltaP = humidity_inside - humidity_outside;

    if (mode != AUTO) {
        fan_running = (mode == ON);

        if (mode.elapsed_millis() >= settings.force_timeout_minutes * 60 * 1000) {
            Serial.printf("Switching back to automatic mode...\n");
            mode = AUTO;
        }
    }

    if (mode == AUTO) {
        if (fan_running && (deltaP <= settings.auto_off_dh)) {
            Serial.printf("Humidity difference dropped, stopping fan.\n");
            fan_running = false;
        } else if (!fan_running && (deltaP >= settings.auto_on_dh)) {
            Serial.printf("Humidity difference raised, starting fan.\n");
            fan_running = true;
        }
    }

    relay.set(fan_running);
}
