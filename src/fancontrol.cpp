#include "Arduino.h"

#include "fancontrol.h"
#include "sensor.h"
#include "settings.h"

FanControl::FanControl(int relay_pin, const Sensors & sensors):
    sensors(sensors), relay_pin(relay_pin), mode(FORCE_OFF) {}

void FanControl::init() {
    pinMode(relay_pin, OUTPUT);
    automatic();
}

void FanControl::force_on() {
    printf("Entering forced on mode.\n");
    mode_time.reset();
    mode = FORCE_ON;
    update_relay();
}

void FanControl::force_off() {
    printf("Entering forced off mode.\n");
    mode_time.reset();
    mode = FORCE_OFF;
    update_relay();
}

void FanControl::automatic() {
    printf("Entering automatic mode...\n");
    if (mode == AUTO_OFF || mode == AUTO_ON) {
        return;
    }
    mode_time.reset();
    const auto deltaP = sensors.inside().humidity - sensors.outside().humidity;
    const auto avg_threshold = (settings::settings.data.auto_on_dh + settings::settings.data.auto_off_dh) / 2;
    if (deltaP >= avg_threshold) {
        printf("High humidity difference, starting fan...\n");
        mode = AUTO_ON;
    } else {
        printf("Low humidity difference, stopping fan...\n");
        mode = AUTO_OFF;
    }
    update_relay();
}

void FanControl::tick() {
    const auto deltaP = sensors.inside().humidity - sensors.outside().humidity;
    switch (mode) {
        case FORCE_ON:
        case FORCE_OFF:
            if (mode_time.elapsed() >= 15 * 60) {
                printf("Switching back to automatic mode...\n");
                automatic();
            }
            break;
        case AUTO_OFF:
            if (deltaP >= settings::settings.data.auto_on_dh) {
                printf("Humidity difference raised, starting fan.\n");
                mode_time.reset();
                mode = AUTO_ON;
                update_relay();
            }
            break;
        case AUTO_ON:
            if (deltaP <= settings::settings.data.auto_off_dh) {
                printf("Humidity difference dropped, stopping fan.\n");
                mode_time.reset();
                mode = AUTO_OFF;
                update_relay();
            }
            break;
    }
}

void FanControl::cycle_modes() {
    switch (mode)
    {
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
    return (mode == AUTO_ON || mode == FORCE_ON);
}

const char * FanControl::mode_str() const {
    switch (mode) {
        case FORCE_ON: return "FOrCE on";
        case FORCE_OFF: return "FOrCE OFF";
        case AUTO_ON: return "Auto on";
        case AUTO_OFF: return "Auto OFF";
        default: return "Error";
    }
}

unsigned int FanControl::current_mode_millis() const {
    return mode_time.elapsed_millis();
}

void FanControl::update_relay() {
    digitalWrite(relay_pin, fan_running() ? HIGH : LOW);
}
