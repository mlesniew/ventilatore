#include "hass.h"

#include <Arduino.h>
#include <PicoHA.h>
#include <PicoMQTT.h>
#include <PicoSyslog.h>
#include <PicoUtils.h>

#include "fancontrol.h"
#include "settings.h"

extern FanControl fan_control;
extern Settings settings;
extern PicoSyslog::SimpleLogger syslog;

namespace {

PicoMQTT::Client mqtt;
PicoHA::Device device(mqtt, "Ventilatore", "mlesniew", "Ventilatore");

}  // namespace

namespace HomeAssistant {

String to_string(FanControl::Mode mode) {
  switch (mode) {
    case FanControl::ON:
      return F("On");
    case FanControl::OFF:
      return F("Off");
    case FanControl::AUTO:
    default:
      return F("Auto");
  }
}

void init() {
  mqtt.host = settings.hass.server;
  mqtt.port = settings.hass.port;
  mqtt.username = settings.hass.username;
  mqtt.password = settings.hass.password;
  mqtt.client_id = "ventilatore-" + String(ESP.getChipId(), HEX);

  PicoHA::add_diagnostic_entities(device);

  auto& fan = device.addFanWithPresetModes<FanControl::Mode, to_string>(
      "fan", nullptr,
      {FanControl::Mode::ON, FanControl::Mode::OFF, FanControl::Mode::AUTO});

  fan.getter = [] { return fan_control.is_fan_running(); };
  fan.setter = [](bool state) {
    fan_control.mode = state ? FanControl::ON : FanControl::OFF;
  };

  fan.preset_mode_getter = [] { return (FanControl::Mode)fan_control.mode; };
  fan.preset_mode_setter = [](FanControl::Mode mode) {
    fan_control.mode = mode;
  };

  auto& target_humidity =
      device.addNumber<double>(F("target_humidity"), F("Target Humidity"));
  target_humidity.bind(&(settings.fan.humidity));
  target_humidity.min = 20;
  target_humidity.max = 80;
  target_humidity.step = 1;

  auto& humidity_hysteresis = device.addNumber<double>(
      F("humidity_hysteresis"), F("Humidity Hysteresis"));
  humidity_hysteresis.bind(&(settings.fan.hysteresis));
  humidity_hysteresis.min = 5;
  humidity_hysteresis.max = 20;
  humidity_hysteresis.step = 1;

  mqtt.begin();
  device.begin();
}

void loop() {
  mqtt.loop();
  device.loop();
}

bool connected() { return mqtt.connected(); }

}  // namespace HomeAssistant
