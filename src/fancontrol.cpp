#include <limits>

#include "Arduino.h"

#include "fancontrol.h"
#include "settings.h"

FanControl::FanControl(PicoUtils::BinaryOutput & relay, const Settings & settings, PicoMQTT::Client & mqtt)
    : relay(relay), settings(settings), mqtt(mqtt), state(FORCE_OFF),
      humidity_inside(std::numeric_limits<double>::quiet_NaN()),
      humidity_outside(std::numeric_limits<double>::quiet_NaN()) {
}

DynamicJsonDocument FanControl::get_json() const {
    StaticJsonDocument<512> json;
    json["fan_running"] = fan_running();
    json["inside"]["humidity"] = (double) humidity_inside;
    json["outside"]["humidity"] = (double) humidity_outside;
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

    automatic();
}

void FanControl::force_on() {
    Serial.printf("Entering forced on state.\n");
    state = FORCE_ON;
    update_relay();
}

void FanControl::force_off() {
    Serial.printf("Entering forced off state.\n");
    state = FORCE_OFF;
    update_relay();
}

void FanControl::automatic() {
    Serial.printf("Entering automatic state...\n");
    if (state == AUTO_OFF || state == AUTO_ON) {
        return;
    }
    const auto deltaP = humidity_inside - humidity_outside;
    const auto avg_threshold = (settings.auto_on_dh + settings.auto_off_dh) / 2;
    if (deltaP >= avg_threshold) {
        printf("High humidity difference, starting fan...\n");
        state = AUTO_ON;
    } else {
        printf("Low humidity difference, stopping fan...\n");
        state = AUTO_OFF;
    }
    update_relay();
}

void FanControl::tick() {
    const auto deltaP = humidity_inside - humidity_outside;
    switch (state) {
        case FORCE_ON:
        case FORCE_OFF:
            if (state.elapsed_millis() >= settings.force_timeout_minutes * 60 * 1000) {
                Serial.printf("Switching back to automatic state...\n");
                automatic();
            }
            break;
        case AUTO_OFF:
            if (deltaP >= settings.auto_on_dh) {
                Serial.printf("Humidity difference raised, starting fan.\n");
                state = AUTO_ON;
                update_relay();
            }
            break;
        case AUTO_ON:
            if (deltaP <= settings.auto_off_dh) {
                Serial.printf("Humidity difference dropped, stopping fan.\n");
                state = AUTO_OFF;
                update_relay();
            }
            break;
    }
}

void FanControl::cycle_modes() {
    switch (state) {
        case FORCE_ON:
            force_off();
            break;
        case FORCE_OFF:
            automatic();
            break;
        default: /* auto modes */
            force_on();
    }
}

bool FanControl::fan_running() const {
    return (state == AUTO_ON || state == FORCE_ON);
}

void FanControl::update_relay() {
    relay.set(fan_running());
}
