#pragma once

#include <PicoUtils.h>
#include <PicoMQ.h>

#include "settings.h"

class FanControl {
    public:
        enum Mode {
            OFF,
            ON,
            AUTO
        };

        FanControl(PicoUtils::BinaryOutput & relay, const Settings & settings, PicoMQ & picomq);

        void init();
        void tick();

        JsonDocument get_json() const;
        PicoUtils::TimedValue<Mode> mode;

        bool is_fan_running() const { return fan_running; }
        bool healthcheck() const { return humidity.elapsed_millis() <= 60 * 3; }

    protected:
        PicoUtils::TimedValue<bool> fan_running;

        PicoUtils::BinaryOutput & relay;
        const Settings & settings;
        PicoMQ & picomq;

        PicoUtils::TimedValue<double> humidity;

        PicoUtils::PeriodicRun periodic;
        PicoUtils::Watch<Mode> mode_watch;

        void process(bool respect_min_times = true);
};
