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

    // CLIENT: set this to the reference ESP32 address.
    config.Mode = Sockets::SocketClockSynchronizationMode::Client;
    config.LocalPort = 45101;
    config.ReferenceAddress = IPAddress(192, 168, 1, 50);
    config.ReferencePort = 45100;
    config.SynchronizationIntervalMilliseconds = 5000;

    // On the reference device instead use:
    // config.Mode = Sockets::SocketClockSynchronizationMode::Reference;
    // config.LocalPort = 45100;

    synchronizer.Initialize(config);
}

void loop() {
    const auto status = synchronizer.GetSynchronizationStatus();
    Serial.printf("state=%u offset=%lld ns delay=%llu ns\n",
        static_cast<unsigned>(status.State),
        static_cast<long long>(status.LastMeasuredOffsetNanoseconds),
        static_cast<unsigned long long>(status.LastRoundTripDelayNanoseconds));
    delay(5000);
}
