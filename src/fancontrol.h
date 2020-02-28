#ifndef FANCONTROL_H
#define FANCONTROL_H

#include "stopwatch.h"

class Sensors;

class FanControl {
    public:
        enum FanMode {
            FORCE_OFF,
            FORCE_ON,
            AUTO_OFF,
            AUTO_ON
        };

        FanControl(int relay_pin, const Sensors & sensors);

        void init();

        void force_on();
        void force_off();
        void automatic();

        void tick();
        void cycle_modes();

        bool fan_running() const;
        const char * mode_str() const;
        unsigned int current_mode_millis() const;
        FanMode get_mode() const { return mode; }

    protected:
        void update_relay();

        FanMode mode;
        const Sensors & sensors;
        const int relay_pin;
        float threshold_off, threshold_on;

        Stopwatch mode_time;
};

#endif
