#include "sensor.h"

#include <Wire.h>
#include <BME280I2C.h>

Sensor::Sensor(unsigned int address) :
    address(address),
    sensor(BME280I2C::Settings(
                BME280::OSR_X1,
                BME280::OSR_X1,
                BME280::OSR_X1,
                BME280::Mode_Forced,
                BME280::StandbyTime_1000ms,
                BME280::Filter_Off,
                BME280::SpiEnable_False,
                address)) {
}

int Sensor::init() {
    if (!sensor.begin()) {
        printf("Could not find BMP280 or BME280 sensor at address 0x%02x.\n", address);
        return 0;
    }
    switch(sensor.chipModel())
    {
        case BME280::ChipModel_BME280:
            printf("Found BME280 sensor at address 0x%02x.\n", address);
            break;
        case BME280::ChipModel_BMP280:
            printf("Found BMP280 sensor at address 0x%02x.  No humidity available.\n", address);
        default:
            printf("Unsupported sensor at address 0x%02x.\n", address);
            return 0;
    }
    update();
    return 1;
}

void Sensor::update() {
    sensor.read(measurements.pressure, measurements.temperature, measurements.humidity,
                BME280::TempUnit_Celsius, BME280::PresUnit_hPa);
    printf("Sensor BME280 0x%02x    T:  %.1f °C    H: %.1f %%    P: %.1f hPa\n",
           address, measurements.temperature, measurements.humidity, measurements.pressure);
}

Sensors::Sensors() :
    sensor_inside(0x76), sensor_outside(0x77) {}

int Sensors::init() {
    Wire.begin();
    Wire.setClock(1000); // use slow speed mode
    return sensor_inside.init() && sensor_outside.init();
}

void Sensors::update() {
    sensor_inside.update();
    sensor_outside.update();
}

const Measurements & Sensors::inside() const {
    return sensor_inside.get_measurements();
}

const Measurements & Sensors::outside() const {
    return sensor_outside.get_measurements();
}
