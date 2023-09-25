#include <ArduinoOTA.h>

void setup() {
    Serial.begin(115200);
    Serial.print(F("\n\n"
                   "Ventilatore updater " __DATE__ " " __TIME__
                   "\n\n"));

    WiFi.softAPdisconnect(true);
    WiFi.begin();

    ArduinoOTA.setHostname("ventilatore");
    ArduinoOTA.begin();
}

void loop() {
    ArduinoOTA.handle();
}
