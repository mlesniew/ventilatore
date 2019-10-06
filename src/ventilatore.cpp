#include <WiFiManager.h>
#include <ESP8266WebServer.h>

#define HOSTNAME "ventilatore"

ESP8266WebServer server;

void setup()
{
    Serial.begin(9600);
    WiFi.hostname(HOSTNAME);

    WiFiManager wifiManager;
    wifiManager.autoConnect(HOSTNAME, "secret");

    server.on("/", []{
            server.send(200, "text/plain", "Hello world!");
            });

    server.begin();
}

void loop()
{
    server.handleClient();
}
