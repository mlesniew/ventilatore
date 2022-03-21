#ifndef SETTINGS_H
#define SETTINGS_H

#include <cstdint>

namespace settings {

    constexpr unsigned int SENSOR_NAME_MAX_LENGTH = 64;

    struct Settings {
        struct {
            unsigned char auto_on_dh;
            unsigned char auto_off_dh;
            unsigned int sensor_check_interval;
            unsigned int altitude;
            char inside_sensor_name[SENSOR_NAME_MAX_LENGTH];
            char outside_sensor_name[SENSOR_NAME_MAX_LENGTH];
        } data;
        uint16_t checksum;
    } __attribute((packed));

    extern Settings settings;

    void sanitize();
    void load();
    void save();
    void print();
}

#endif
