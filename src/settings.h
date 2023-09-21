#pragma once

#include <ArduinoJson.h>

struct Settings {
    double auto_on_dh;
    double auto_off_dh;

    String inside_sensor_address;
    String outside_sensor_address;

    unsigned int force_timeout_minutes;

    DynamicJsonDocument get_json() const;
    void load(const JsonDocument & json);
    void load();

    void sanitize();
    void save() const;
    void print();
};
