#include <Arduino.h>
#include <WiFi.h>
#include <ESPressio_SocketClockSynchronization.hpp>

using namespace ESPressio;
const char* SSID = "YOUR_WIFI";
const char* PASSWORD = "YOUR_PASSWORD";
Sockets::UDPClockSynchronizer synchronizer;

void setup() {
    Serial.begin(115200);
    WiFi.begin(SSID, PASSWORD);
    while (WiFi.status() != WL_CONNECTED) delay(250);

    Sockets::UDPClockSynchronizationConfig config;
    config.LocalPort = 45100;

    // Reference device:
    config.Mode = Sockets::SocketClockSynchronizationMode::Reference;
    config.EnableAuthoritativeBroadcast = true;
    config.BroadcastIntervalMilliseconds = 5000;

    // Receiver devices instead use:
    // config.Mode = Sockets::SocketClockSynchronizationMode::Client;
    // and simply listen on LocalPort 45100.

    synchronizer.Initialize(config);
}

void loop() { delay(1000); }
