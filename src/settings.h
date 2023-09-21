#pragma once

#include <ArduinoJson.h>

struct Settings {
    struct {
        double auto_on_dh;
        double auto_off_dh;
        unsigned int force_timeout_minutes;
    } fan;

    struct {
        String inside;
        String outside;
    } sensors;

    struct {
        String server;
        uint16_t port;
        String username;
        String password;
    } mqtt;

    DynamicJsonDocument get_json() const;
    void load(const JsonDocument & json);

    void sanitize();
    void print();
};
