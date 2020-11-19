![logo](img/logo.png)

[Ventilatore](https://github.com/mlesniew/ventilatore) is an automatic bathroom fan controller built using the NodeMCU.

[![Build Status](https://travis-ci.org/mlesniew/ventilatore.svg?branch=master)](https://travis-ci.org/mlesniew/ventilatore)
![License](https://img.shields.io/github/license/mlesniew/ventilatore)

## How it works

The air humidity inside and outside the bathroom is measured using two
[Bosch BME280](https://www.bosch-sensortec.com/products/environmental-sensors/humidity-sensors-bme280/) sensors.
If the humidity difference (inside minus outside) raises above a given threshold, the fan gets started through a
relay.  The fan is kept running until the humidity difference drops to an acceptable value.

## Pictures

![Outside](/img/outside.jpg)

![Inside](/img/inside.jpg)

## Web interface

If connected to a WiFi network, a web interface is exposed with live measurements and configuration options.

![Web interface](/img/screenshot.png)

## Prometheus metrics

Besides the web page, a [Prometheus](https://prometheus.io/) metrics endpoint is exposed.
These metrics can be then be visualized on [Grafana](https://grafana.com/) dashboards:

![Grafana](/img/grafana.png)

[Grafana dashboard configuration](/grafana.json)

## Electronics

### Required parts
* NodeMCU
* Bosch BME280 sensor x2
* 5V relay
* TM1637 4-digit 7-segment display
* MTS on-off-on toggle switch
* LEDs: red and blue
* Resistors: 220 Ω, 100 Ω, 2x 4,7 kΩ
* Monostable push button

### Schema
![Schema](/img/schema.png)

## Compilation

The project can be built using [PlatformIO](https://platformio.org/).  Nothing else needs to be installed, all
required libraries are linked to the project as git submodules.

Steps to download, build and upload:
```
git clone https://github.com/mlesniew/ventilatore.git
cd ventilatore
git submodule init
git submodule update
pio run
pio run --target=upload
pio run --target=uploadfs
```
