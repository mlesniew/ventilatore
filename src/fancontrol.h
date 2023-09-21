#pragma once

#include <PicoUtils.h>
#include <PicoMQTT.h>

#include "settings.h"

class FanControl {
    public:
        enum State {
            FORCE_OFF,
            FORCE_ON,
            AUTO_OFF,
            AUTO_ON
        };

        FanControl(PicoUtils::BinaryOutput & relay, const Settings & settings, PicoMQTT::Client & mqtt);

        void init();

        void force_on();
        void force_off();
        void automatic();

        void tick();
        void cycle_modes();

        bool fan_running() const;
        State get_state() const { return state; }

        DynamicJsonDocument get_json() const;

    protected:
        PicoUtils::BinaryOutput & relay;
        const Settings & settings;
        PicoMQTT::Client & mqtt;

        PicoUtils::TimedValue<State> state;

        PicoUtils::TimedValue<double> humidity_inside;
        PicoUtils::TimedValue<double> humidity_outside;

        void update_relay();
};
