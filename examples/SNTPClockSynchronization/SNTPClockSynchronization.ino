#include <Arduino.h>
#include <WiFi.h>
#include <ESPressio_SocketClockSynchronization.hpp>
using namespace ESPressio;
const char* SSID="YOUR_WIFI"; const char* PASSWORD="YOUR_PASSWORD";
Sockets::SNTPClockSyncProvider sntp;
void setup(){ Serial.begin(115200); WiFi.begin(SSID,PASSWORD); while(WiFi.status()!=WL_CONNECTED) delay(250); Sockets::SNTPClockSyncProviderConfig c; c.Server="pool.ntp.org"; c.UpdateIntervalMilliseconds=3600000; sntp.Initialize(c); }
void loop(){ const auto s=sntp.GetSynchronizationStatus(); Serial.printf("state=%u accepted=%lu offset=%lld ns\n", static_cast<unsigned>(s.State), static_cast<unsigned long>(s.AcceptedSampleCount), static_cast<long long>(s.LastMeasuredOffsetNanoseconds)); delay(5000); }
