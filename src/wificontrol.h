#ifndef WIFI_CONTROL_H
#define WIFI_CONTROL_H

class BlinkingLed;

class WiFiControl {
    public:
        WiFiControl(const char * hostname, BlinkingLed & led):
            hostname(hostname), led(led), previous_wifi_status(WL_NO_SHIELD) {}
        void init(bool setup=false);
        void tick();

    protected:
        const char * hostname;
        BlinkingLed & led;
        wl_status_t previous_wifi_status;
};

#endif
