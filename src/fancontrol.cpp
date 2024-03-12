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
      humidity(std::numeric_limits<double>::quiet_NaN()),
      periodic(5, [this] { process(); }),
mode_watch([this] { return mode; }, [this] { process(false); }) {
}

JsonDocument FanControl::get_json() const {
    JsonDocument json;
    json["fan_running"] = (bool) fan_running;
    json["humidity"] = (double) humidity;
    switch (mode) {
        case ON: json["mode"] = "on"; break;
        case OFF: json["mode"] = "off"; break;
        case AUTO: json["mode"] = "auto"; break;
    }
    json["target_humidity"] = settings.fan.humidity;
    json["hysteresis"] = settings.fan.hysteresis;
    return json;
}

void FanControl::init() {
    if (settings.sensor.length()) {
        picomq.subscribe("celsius/+/" + settings.sensor + "/humidity", [this](String payload) {
            const bool first_reading = std::isnan(humidity);
            humidity = payload.toDouble();
            if (first_reading) {
                process(false);
            }
        });
    }

    metrics::fan_running.bind([this] { return fan_running ? 1 : 0; });
    metrics::air_humidity.bind([this] { return (double) humidity; });

    mode = AUTO;
}

void FanControl::tick() {
    mode_watch.tick();
    periodic.tick();
}

void FanControl::process(bool respect_min_times) {
    if (mode != AUTO) {
        fan_running = (mode == ON);

        if (mode.elapsed_millis() >= settings.fan.force_timeout_minutes * 60 * 1000) {
            logger.println(F("Switching back to automatic mode..."));
            mode = AUTO;
        }
    }

    const auto auto_off_humidity = settings.fan.humidity - settings.fan.hysteresis / 2;
    const auto auto_on_humidity = settings.fan.humidity + settings.fan.hysteresis / 2;

    const bool wet = (humidity > auto_on_humidity);
    const bool dry = (humidity <= auto_off_humidity);

    if (mode == AUTO) {
        unsigned int elapsed = fan_running.elapsed_millis() / (60 * 1000);

        if (fan_running) {
            const bool min_time_elapsed = (elapsed >= settings.fan.min_auto_on_time);
            const bool max_time_elapsed = (elapsed >= settings.fan.max_auto_on_time) && (settings.fan.max_auto_on_time > 0);

            if (max_time_elapsed) {
                logger.println(F("Max fan run time reached, stopping fan."));
                fan_running = false;
            }

            if (respect_min_times && !min_time_elapsed) {
                return;
            }

            if (dry) {
                logger.println(F("Humidity dropped, stopping fan."));
                fan_running = false;
            }
        } else {
            const bool min_time_elapsed = (elapsed >= settings.fan.min_auto_off_time);
            const bool max_time_elapsed = (elapsed >= settings.fan.max_auto_off_time) && (settings.fan.max_auto_off_time > 0);

            if (max_time_elapsed) {
                logger.println(F("Max fan inactivity time reached, starting fan."));
                fan_running = true;
            }

            if (respect_min_times && !min_time_elapsed) {
                return;
            }

            if (wet) {
                fan_running = true;
            }
        }
    }

    relay.set(fan_running);
}
