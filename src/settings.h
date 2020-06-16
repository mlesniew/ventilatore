#ifndef SETTINGS_H
#define SETTINGS_H

#include <cstdint>

namespace settings {

    struct Settings {
        struct {
            unsigned char auto_on_dh;
            unsigned char auto_off_dh;
            unsigned int sensor_check_interval;
            unsigned int altitude;
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
