#ifndef LED_H
#define LED_H

#include "stopwatch.h"

struct BlinkingLed {
    BlinkingLed(unsigned int pin, unsigned long pattern=0b10, unsigned long interval=500, bool inverted=false)
        : pin(pin), inverted(inverted), pattern(pattern), interval(interval) {
        pinMode(pin, OUTPUT);
        restart_pattern();
        next();
    }

    void set_pattern(unsigned long pattern) {
        if (this->pattern != pattern) {
            this->pattern = pattern;
            restart_pattern();
        }
    }

    unsigned long get_pattern() const {
        return this->pattern;
    }

    void tick() {
        if (stopwatch.elapsedMillis() > interval) {
            next();
            stopwatch.reset();
        }
    }

    void restart_pattern() {
        for (position = 31; position > 0; --position) {
            if ((pattern >> position) & 1)
                break;
        }
    }

    void next() {
        bool led_on = (pattern >> position) & 1;
        digitalWrite(pin, (led_on != inverted) ? HIGH : LOW);
        if (position-- == 0)
            restart_pattern();
    }

    unsigned int pin;
    bool inverted;
    unsigned long pattern;
    unsigned long interval;
    unsigned char position;
    Stopwatch stopwatch;
};

#endif
