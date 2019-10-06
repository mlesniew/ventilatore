#include <WiFiManager.h>
#include <ESP8266WebServer.h>
#include <FS.h>

#define HOSTNAME "ventilatore"
#define RELAY D1

ESP8266WebServer server;

void setup()
{
    Serial.begin(9600);
    SPIFFS.begin();

    pinMode(RELAY, OUTPUT);
    digitalWrite(RELAY, LOW);

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

    server.on("/on", []{
            digitalWrite(RELAY, HIGH);
            server.send(200, "text/plain", "OK");
            });

    server.on("/off", []{
            digitalWrite(RELAY, LOW);
            server.send(200, "text/plain", "OK");
            });

    server.begin();
}

void loop()
{
    server.handleClient();
}
