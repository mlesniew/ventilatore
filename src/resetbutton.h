#include <Ticker.h>

#include "stopwatch.h"

class ResetButton {
public:
    ResetButton(unsigned int pin, float timeout = 20.0);

    void init();
    void tick();
    bool pressed() const;

    const unsigned int pin;
    const float timeout;

protected:
    Ticker ticker;
    Stopwatch stopwatch;
};
