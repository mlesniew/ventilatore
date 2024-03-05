#pragma once

#include <ArduinoJson.h>

struct Settings {
    struct {
        String hostname;
        String ota_password;
        String syslog;
    } net;

    struct {
        double auto_on_humidity;
        double auto_off_humidity;
        unsigned int force_timeout_minutes;
        unsigned int max_auto_on_time;
        unsigned int min_auto_on_time;
    } fan;

    String sensor;

    struct {
        String server;
        uint16_t port;
        String username;
        String password;
    } mqtt, hass;

    DynamicJsonDocument get_json() const;
    void load(const JsonDocument & json);

    void sanitize();
    void print();
};
