#include <limits>

#include <Arduino.h>
#include <PicoPrometheus.h>

#include "fancontrol.h"
#include "settings.h"

PicoPrometheus::Registry & get_prometheus();
extern Print & logger;

namespace metrics {
PicoPrometheus::Gauge fan_running(get_prometheus(), "fan_running", "Fan running state");
PicoPrometheus::Gauge air_humidity(get_prometheus(), "air_humidity", "Relative humidity in percent");
}

FanControl::FanControl(PicoUtils::BinaryOutput & relay, const Settings & settings, PicoMQ & picomq)
    : mode(OFF), fan_running(false), relay(relay), settings(settings), picomq(picomq),
      humidity(std::numeric_limits<double>::quiet_NaN()) {

}

DynamicJsonDocument FanControl::get_json() const {
    StaticJsonDocument<512> json;
    json["fan_running"] = fan_running;
    json["humidity"] = (double) humidity;
    switch (mode) {
        case ON: json["mode"] = "on"; break;
        case OFF: json["mode"] = "off"; break;
        case AUTO: json["mode"] = "auto"; break;
    }
    return json;
}

void FanControl::init() {
    if (settings.sensor.length()) {
        picomq.subscribe("celsius/+/" + settings.sensor + "/humidity", [this](String payload) {
            humidity = payload.toDouble();
            logger.printf("Humidity updated to %.1f %%\n", (double) humidity);
        });
    }

    metrics::fan_running.bind([this] { return fan_running ? 1 : 0; });
    metrics::air_humidity.bind([this] { return (double) humidity; });

    mode = AUTO;
}

void FanControl::tick() {
    if (mode != AUTO) {
        fan_running = (mode == ON);

        if (mode.elapsed_millis() >= settings.fan.force_timeout_minutes * 60 * 1000) {
            logger.println(F("Switching back to automatic mode..."));
            mode = AUTO;
        }
    }

    if (mode == AUTO) {
        if (fan_running && (humidity <= settings.fan.auto_off_humidity)) {
            logger.println(F("Humidity dropped, stopping fan."));
            fan_running = false;
        } else if (!fan_running && (humidity > settings.fan.auto_on_humidity)) {
            logger.println(F("Humidity raised, starting fan."));
            fan_running = true;
        }
    }

    relay.set(fan_running);
}
