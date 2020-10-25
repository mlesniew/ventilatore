#ifndef SENSOR_H
#define SENSOR_H

#include <BME280I2C.h>

struct Measurements {
    float temperature, humidity, pressure;
    bool valid() const {
        return (humidity <= 100) && (humidity >= 0);
    }
};

class Sensor {
    public:
        Sensor(unsigned int address);
        bool init();
        bool update();
        const Measurements & get_measurements() const { return measurements; }

    protected:
        const unsigned int address;
        BME280I2C sensor;
        Measurements measurements;
};

class Sensors {
public:
    Sensors(const int reset_pin);
    bool init();
    bool update();

    const Measurements & inside() const;
    const Measurements & outside() const;

protected:
    const int reset_pin;
    Sensor sensor_inside, sensor_outside;
};

#endif
