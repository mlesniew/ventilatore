#include "sensor.h"

#include <Wire.h>
#include <BME280I2C.h>

Sensor::Sensor(BME280I2C::I2CAddr address) :
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

bool Sensor::init() {
    if (!sensor.begin()) {
        printf("Could not find BMP280 or BME280 sensor at address 0x%02x.\n", int(address));
        return false;
    }
    switch(sensor.chipModel())
    {
        case BME280::ChipModel_BME280:
            printf("Found BME280 sensor at address 0x%02x.\n", int(address));
            break;
        case BME280::ChipModel_BMP280:
            printf("Found BMP280 sensor at address 0x%02x.  No humidity available.\n", int(address));
        default:
            printf("Unsupported sensor at address 0x%02x.\n", int(address));
            return false;
    }
    return update();
}

bool Sensor::update() {
    sensor.read(measurements.pressure, measurements.temperature, measurements.humidity,
                BME280::TempUnit_Celsius, BME280::PresUnit_hPa);
    printf("Sensor BME280 0x%02x    T:  %.1f °C    H: %.1f %%    P: %.1f hPa\n",
           int(address), measurements.temperature, measurements.humidity, measurements.pressure);
    return measurements.valid();
}

Sensors::Sensors(const int reset_pin) :
    reset_pin(reset_pin), sensor_inside(BME280I2C::I2CAddr_0x76), sensor_outside(BME280I2C::I2CAddr_0x77) {}

bool Sensors::init() {
    // The sensors are powered through the reset_pin.  Here we're setting
    // it to low and high again to ensure the I2C sensors are fully reset.
    // This is needed because sometimes the I2C chips hang and just a simple
    // reinitialization doesn't bring them back to life.
    pinMode(reset_pin, OUTPUT);
    digitalWrite(reset_pin, 0);
    delay(1000);
    digitalWrite(reset_pin, 1);
    Wire.begin();
    Wire.setClock(1000); // use slow speed mode

    bool all_success = true;
    if (!sensor_outside.init()){
        all_success = false;
        printf("Failed to initialize sensor from outside.\n");
    }
    if (!sensor_inside.init()){
        all_success = false;
        printf("Failed to initialize sensor from inside.\n");
    }
    return all_success;
}

bool Sensors::update() {
    bool all_success = true;
    if (!sensor_outside.update()){
        all_success = false;
        printf("Failed to update sensor from outside.\n");
    }
    if (!sensor_inside.update()){
        all_success = false;
        printf("Failed to update sensor from inside.\n");
    }
    return all_success;
}

const Measurements & Sensors::inside() const {
    return sensor_inside.get_measurements();
}

const Measurements & Sensors::outside() const {
    return sensor_outside.get_measurements();
}
