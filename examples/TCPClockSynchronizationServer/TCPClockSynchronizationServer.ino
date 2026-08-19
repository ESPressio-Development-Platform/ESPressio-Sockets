#include <Arduino.h>
#include <WiFi.h>
#include <ESPressio_SocketClockSynchronization.hpp>
using namespace ESPressio;
const char* SSID="YOUR_WIFI"; const char* PASSWORD="YOUR_PASSWORD";
Sockets::TCPClockSynchronizationServer server;
void setup(){ Serial.begin(115200); WiFi.begin(SSID,PASSWORD); while(WiFi.status()!=WL_CONNECTED) delay(250); Sockets::TCPClockSynchronizationServerConfig c; c.Port=45110; server.Initialize(c); Serial.println(WiFi.localIP()); }
void loop(){ delay(1000); }
