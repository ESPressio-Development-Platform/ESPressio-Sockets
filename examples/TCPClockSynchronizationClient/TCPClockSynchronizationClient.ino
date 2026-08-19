#include <Arduino.h>
#include <WiFi.h>
#include <ESPressio_SocketClockSynchronization.hpp>
using namespace ESPressio;
const char* SSID="YOUR_WIFI"; const char* PASSWORD="YOUR_PASSWORD";
Sockets::TCPClockSynchronizationClient client;
void setup(){ Serial.begin(115200); WiFi.begin(SSID,PASSWORD); while(WiFi.status()!=WL_CONNECTED) delay(250); Sockets::TCPClockSynchronizationClientConfig c; c.Host="192.168.1.50"; c.Port=45110; c.SynchronizationIntervalMilliseconds=5000; client.Initialize(c); }
void loop(){ delay(1000); }
