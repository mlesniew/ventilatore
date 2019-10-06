#include <WiFiManager.h>
#include <ESP8266WebServer.h>

ESP8266WebServer server;

void setup()
{
    Serial.begin(9600);

    WiFiManager wifiManager;
    wifiManager.autoConnect("ventilatore", "secret");

    server.on("/", []{
            server.send(200, "text/plain", "Hello world!");
            });

    server.begin();
}

void loop()
{
    server.handleClient();
}
