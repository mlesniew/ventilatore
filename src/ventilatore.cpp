#include <WiFiManager.h>

void setup()
{
    Serial.begin(9600);

    WiFiManager wifiManager;
    wifiManager.autoConnect("ventilatore", "secret");
}

void loop()
{

}
