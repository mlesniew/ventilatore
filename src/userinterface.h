#ifndef USERINTERFACE_H
#define USERINTERFACE_H

#include "stopwatch.h"
#include "fancontrol.h"

class FanControl;
class OneButton;
class ScrollingDisplay;
class Sensors;

class UserInterface {
public:
    enum Mode {
        HUMIDITY_INSIDE_SIMPLE,
        HUMIDITY_INSIDE,
        HUMIDITY_OUTSIDE,
        TEMPERATURE_INSIDE,
        TEMPERATURE_OUTSIDE,
        PRESSURE_INSIDE,
        PRESSURE_OUTSIDE,
        FAN_MODE,
    };

    UserInterface(ScrollingDisplay & display, FanControl & fan, const Sensors & sensors, OneButton & button);
    void update_display(bool immediate);
    void tick();

protected:
    void on_click();
    void on_long_click();

    // These wrappers are needed because OneButton only accepts C function ptrs, so no lambdas with captured
    // values are allowed.  Klingt komisch, ist aber so.
    static void on_click(void * self) {
        ((UserInterface *)(self))->on_click();
    }
    static void on_long_click(void * self) {
        ((UserInterface *)(self))->on_long_click();
    }

    ScrollingDisplay & display;
    FanControl & fan;
    const Sensors & sensors;
    OneButton & button;
    FanControl::FanMode last_fan_mode;

    Stopwatch last_button_press, last_display_update, last_brightness_update;
    Mode mode, last_mode;

    unsigned char brightness;
    bool display_on;
};

#endif
