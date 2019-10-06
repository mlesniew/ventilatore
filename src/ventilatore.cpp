#include <WiFiManager.h>
#include <ESP8266WebServer.h>
#include <FS.h>

#define HOSTNAME "ventilatore"

ESP8266WebServer server;

void setup()
{
    Serial.begin(9600);
    SPIFFS.begin();

    WiFi.hostname(HOSTNAME);

    WiFiManager wifiManager;
    wifiManager.autoConnect(HOSTNAME, "secret");

    server.on("/version", []{
            server.send(200, "text/plain", __DATE__ " " __TIME__);
            });

    server.on("/uptime", []{
            server.send(200, "text/plain", String(millis() / 1000));
            });

    server.serveStatic("/", SPIFFS, "/index.html");

    server.begin();
}

void loop()
{
    server.handleClient();
}
