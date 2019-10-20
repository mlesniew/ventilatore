#include <ESP8266WebServer.h>
#include <FS.h>
#include <TM1637Display.h>
#include <WiFiManager.h>

#define HOSTNAME "ventilatore"
#define RELAY D1

ESP8266WebServer server;

static const uint8_t lowercaseToSegment[] = {
    0b1011111, 0b1111100, 0b1011000, 0b1011110, 0b1111011, 0b1110001,
    0b1101111, 0b1110100, 0b0000100, 0b0001110, 0b1110100, 0b0110000,
    0b1010101, 0b1010100, 0b1011100, 0b1110011, 0b1100111, 0b1010000,
    0b1101101, 0b1111000, 0b0011100, 0b1100010, 0b1101010, 0b1010010,
    0b1100110, 0b1011011
};

static const uint8_t uppercaseToSegment[] = {
    0b1110111, 0b1111111, 0b0111001, 0b0111111, 0b1111001, 0b1110001,
    0b0111101, 0b1110110, 0b0000110, 0b0011110, 0b1110110, 0b0111000,
    0b1010101, 0b1010100, 0b0111111, 0b1110011, 0b1100111, 0b1110111,
    0b1101101, 0b0110001, 0b0111110, 0b1110010, 0b1111110, 0b1010010,
    0b1101110, 0b1011011
};

class Display : public TM1637Display
{
public:
    using TM1637Display::TM1637Display;

    void showText(const char * text)
    {
        uint8_t segments[] = { 0, 0, 0, 0 };
        for (int i = 0; i < 4 && text[i]; ++i) {
            segments[i] = encodeChar(text[i]);
        }
        setSegments(segments);
    }

    uint8_t encodeChar(char c)
    {
        if (c >= '0' && c <= '9')
            return encodeDigit(c - '0');

        if (c >= 'a' && c <= 'z')
            return lowercaseToSegment[c - 'a'];

        if (c >= 'A' && c <= 'Z')
            return uppercaseToSegment[c - 'A'];

        if (c == '-')
            return 0b1000000;

        return 0;
    }

};

Display display(D6 /* clk */, D5 /* dio */);

void setup()
{
    Serial.begin(9600);
    display.clear();
    SPIFFS.begin();

    display.setBrightness(7);
    display.showText("bOOt");

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
            display.showText(" on ");
            server.send(200, "text/plain", "OK");
            });

    server.on("/off", []{
            digitalWrite(RELAY, LOW);
            display.showText(" oFF");
            server.send(200, "text/plain", "OK");
            });

    server.begin();

    display.showText("LOOP");
}

void loop()
{
    server.handleClient();
    display.setBrightness(1);
}
