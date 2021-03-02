#include <Arduino.h>

#include <utils/reset.h>

#include "resetbutton.h"

ResetButton::ResetButton(unsigned int pin, float timeout) : pin(pin), timeout(timeout) {
}

void ResetButton::init() {
    printf("Holding down button at pin %u for %.1f seconds will cause a soft reset.\n",
           pin, timeout);
    ticker.attach(0.2, [this]{ tick(); });
}

bool ResetButton::pressed() const {
    return digitalRead(D0) == HIGH;
}

void ResetButton::tick() {
    if (!pressed()) {
        stopwatch.reset();
        return;
    }

    // button is pressed, check stopwatch
    if (stopwatch.elapsed() >= timeout)
    {
        printf("Software reset requested by holding the button.\n");
        reset();
    }
}


