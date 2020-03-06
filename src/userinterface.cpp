#include <OneButton.h>

#include "fancontrol.h"
#include "scrolling_display.h"
#include "sensor.h"
#include "userinterface.h"

void UserInterface::on_click() {
    switch (mode) {
        case FAN_MODE:
            mode = last_mode != FAN_MODE ? last_mode : HUMIDITY_INSIDE_SIMPLE;
            break;
        case HUMIDITY_INSIDE_SIMPLE:
            mode = HUMIDITY_INSIDE;
            break;
        case HUMIDITY_INSIDE:
            mode = HUMIDITY_OUTSIDE;
            break;
        case HUMIDITY_OUTSIDE:
            mode = TEMPERATURE_INSIDE;
            break;
        case TEMPERATURE_INSIDE:
            mode = TEMPERATURE_OUTSIDE;
            break;
        case TEMPERATURE_OUTSIDE:
            mode = PRESSURE_INSIDE;
            break;
        case PRESSURE_INSIDE:
            mode = PRESSURE_OUTSIDE;
            break;
        case PRESSURE_OUTSIDE:
            mode = HUMIDITY_INSIDE;
            break;
    }
    update_display(true);
    last_button_press.reset();
}

void UserInterface::on_long_click() {
    fan.cycle_modes();
    last_button_press.reset();
}

UserInterface::UserInterface(ScrollingDisplay & display, FanControl & fan, const Sensors & sensors, OneButton & button)
    : display(display), fan(fan), sensors(sensors), button(button), last_fan_mode(fan.get_mode()),
    mode(HUMIDITY_INSIDE_SIMPLE), last_mode(HUMIDITY_INSIDE_SIMPLE) {

    button.attachClick(on_click, (void *)(this));
    button.attachLongPressStart(on_long_click, (void *)(this));
}

void UserInterface::update_display(bool immediate) {
    char buf[200];

    switch (mode) {
        case HUMIDITY_INSIDE:
            snprintf(buf, 200, "Hin %i PErc", int(sensors.inside().humidity));
            break;
        case HUMIDITY_OUTSIDE:
            snprintf(buf, 200, "Hout  %i PErc", int(sensors.outside().humidity));
            break;
        case PRESSURE_INSIDE:
            snprintf(buf, 200, "Pin %i hPA", int(sensors.inside().pressure));
            break;
        case PRESSURE_OUTSIDE:
            snprintf(buf, 200, "Pout  %i hPA", int(sensors.outside().pressure));
            break;
        case TEMPERATURE_INSIDE:
            snprintf(buf, 200, "tin %i *C", int(sensors.inside().temperature));
            break;
        case TEMPERATURE_OUTSIDE:
            snprintf(buf, 200, "tout  %i *C", int(sensors.outside().temperature));
            break;
        case HUMIDITY_INSIDE_SIMPLE:
            snprintf(buf, 200, " %3i", int(sensors.inside().humidity));
            break;
        case FAN_MODE:
            snprintf(buf, 200, "%s", fan.mode_str());
            break;
    }

    if (immediate)
        display.set_text(buf);
    else
        display.set_next_text(buf);

    last_display_update.reset();
}

void UserInterface::tick() {
    button.tick();

    if (last_fan_mode != fan.get_mode()) {
        // fan mode changed -- display immediately
        printf("Fan mode changed, updating display.\n");
        display.set_text(fan.mode_str());
        // remember last display mode (if it wasn't already set to FAN_MODE)
        last_mode = mode != FAN_MODE ? mode : last_mode;
        // set new mode
        mode = FAN_MODE;
        last_fan_mode = fan.get_mode();
        update_display(true);
    }

    switch (mode) {
        case FAN_MODE:
            if (last_display_update.elapsed() >= 10) {
                printf("Fan mode change too long ago\n");
                mode = last_mode != FAN_MODE ? last_mode : HUMIDITY_INSIDE_SIMPLE;
                update_display(false);
            }
            break;
        case HUMIDITY_INSIDE_SIMPLE:
            if (last_display_update.elapsed() >= 5) {
                update_display(false);
            }
            break;
        default:
            if (last_button_press.elapsed() >= 30) {
                // go back to simple mode
                printf("No user activity, switching display back to simple humidity display mode.\n");
                mode = HUMIDITY_INSIDE_SIMPLE;
                update_display(false);
            } else if (last_display_update.elapsed() >= 5) {
                // update displayed measurements
                update_display(false);
            }
            break;
    }
}
