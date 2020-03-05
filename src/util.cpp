#include <Arduino.h>

#include "util.h"

void reset() {
    while (1) {
        printf("Reset...\n");
        ESP.restart();
        printf("Reset failed?  Retrying in 10 seconds...\n");
        delay(10000);
    }
}
