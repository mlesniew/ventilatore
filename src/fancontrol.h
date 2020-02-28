#ifndef FANCONTROL_H
#define FANCONTROL_H

#include "stopwatch.h"

class Sensors;

class FanControl {
    public:
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


    protected:
        void update_relay();

        const Sensors & sensors;
        const int relay_pin;
        float threshold_off, threshold_on;

        enum {
            FORCE_OFF,
            FORCE_ON,
            AUTO_OFF,
            AUTO_ON
        } mode;

        Stopwatch mode_time;
};

#endif
