#ifndef STOPWATCH_H
#define STOPWATCH_H

#include "Arduino.h"

class Stopwatch {
    unsigned long start;

public:
    Stopwatch() { reset(); }

    unsigned long elapsedMillis() const {
        return millis() - start;
    }

    float elapsed() const {
        return float(elapsedMillis()) / 1000.0;
    }

    void reset() {
        start = millis();
    }
};

#endif
