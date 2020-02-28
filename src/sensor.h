#ifndef SENSOR_H
#define SENSOR_H

#include <BME280I2C.h>

struct Measurements {
    float temperature, humidity, pressure;
};

class Sensor {
    public:
        Sensor(unsigned int address);
        int init();
        void update();
        const Measurements & get_measurements() const { return measurements; }

    protected:
        const unsigned int address;
        BME280I2C sensor;
        Measurements measurements;
};

class Sensors {
public:
    Sensors();
    int init();
    void update();

    const Measurements & inside() const;
    const Measurements & outside() const;

protected:
    Sensor sensor_inside, sensor_outside;
};

#endif
