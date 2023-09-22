#pragma once

#include <PicoUtils.h>
#include <PicoMQTT.h>

#include "settings.h"

class FanControl {
    public:
        enum Mode {
            OFF,
            ON,
            AUTO
        };

        FanControl(PicoUtils::BinaryOutput & relay, const Settings & settings, PicoMQTT::Client & mqtt);

        void init();
        void tick();

        DynamicJsonDocument get_json() const;
        PicoUtils::TimedValue<Mode> mode;

        bool is_fan_running() const { return fan_running; }

    protected:
        bool fan_running;

        PicoUtils::BinaryOutput & relay;
        const Settings & settings;
        PicoMQTT::Client & mqtt;

        PicoUtils::TimedValue<double> humidity_inside;
        PicoUtils::TimedValue<double> humidity_outside;

        void update_relay();
};
