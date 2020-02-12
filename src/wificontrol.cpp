#include <WiFiManager.h>

#include "wificontrol.h"
#include "led.h"

void WiFiControl::init(bool setup) {
    BackgroundLedController blc(led);
    led.set_pattern(0b100);
    WiFi.hostname(hostname);
    WiFi.setAutoReconnect(true);
    if (setup) {
        printf("WiFi setup mode requested, starting AP...\n");
        led.set_pattern(0b100100100 << 9);
        WiFiManager wifiManager;
        wifiManager.startConfigPortal(hostname);
    } else {
        printf("WiFi setup mode not requested.\n");
        WiFi.begin("", "");
    }
}

void WiFiControl::tick() {
    wl_status_t current_wifi_status = WiFi.status();

    if (current_wifi_status != previous_wifi_status) {
        printf("WiFi: %i -> %i\n", (int)(previous_wifi_status), (int)(current_wifi_status));
        switch (current_wifi_status) {
            case WL_CONNECTED:
                printf("IP: %s\n", WiFi.localIP().toString().c_str());
                led.set_pattern(0b1 << 60);
                break;
            case WL_DISCONNECTED:
                led.set_pattern(0);
                break;
            default:
                led.set_pattern(0b100);
                break;
        }
        previous_wifi_status = current_wifi_status;
    }

    led.tick();
}
