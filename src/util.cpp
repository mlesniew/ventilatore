#include <Arduino.h>

#include "util.h"

void reset() {
    while (1) {
        printf("Reset...\n");
        while (1) {
            ESP.restart();
            delay(10000);
        }
    }
}
