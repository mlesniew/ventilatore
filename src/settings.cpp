#include <cstdio>
#include <CRC.h>
#include <EEPROM.h>
#include "settings.h"

#define DEFAULT_AUTO_ON_DH 15
#define DEFAULT_AUTO_OFF_DH 20
#define DEFAULT_CHECK_INTERVAL 30
#define DEFAULT_ALTITUDE 0

namespace settings {

Settings settings;

void sanitize() {
    if (settings.data.auto_on_dh > 100)
        settings.data.auto_on_dh = 100;

    if (settings.data.auto_off_dh > 100)
        settings.data.auto_off_dh = 100;

    // auto_on_dh must be grater or equal to auto_off_dh
    if (settings.data.auto_off_dh > settings.data.auto_on_dh) {
        const auto tmp = settings.data.auto_off_dh;
        settings.data.auto_off_dh = settings.data.auto_on_dh;
        settings.data.auto_on_dh = tmp;
    }

    if (settings.data.sensor_check_interval < 5)
        settings.data.sensor_check_interval = 5;

    if (settings.data.sensor_check_interval >= 10 * 60)
        settings.data.sensor_check_interval = 10 * 60;

    if (settings.data.altitude >= 9000)
        settings.data.altitude = 9000;
}

void load() {
    EEPROM.begin(sizeof(Settings));
    EEPROM.get(0, settings);

    const auto expected_checksum = CRC::crc16(&settings.data, sizeof(settings.data));
    if (settings.checksum != expected_checksum) {
        printf("Invalid CRC of settings in flash, using defaults.\n");
        settings.data.auto_on_dh = DEFAULT_AUTO_ON_DH;
        settings.data.auto_off_dh = DEFAULT_AUTO_OFF_DH;
        settings.data.sensor_check_interval = DEFAULT_CHECK_INTERVAL;
        settings.data.altitude = DEFAULT_ALTITUDE;
    } else {
        printf("Loaded settings from flash, CRC correct.\n");
    }

    sanitize();
    print();
}

void save() {
    settings.checksum = CRC::crc16(&settings.data, sizeof(settings.data));
    EEPROM.put(0, settings);
    EEPROM.commit();
}

void print() {
    printf("Settings:\n");
    printf("  auto on humidity difference:  %i\n", settings.data.auto_on_dh);
    printf("  auto off humidity difference: %i\n", settings.data.auto_off_dh);
    printf("  sensor check interval:        %i\n", settings.data.sensor_check_interval);
    printf("  altitude:                     %i\n", settings.data.altitude);
}

}
