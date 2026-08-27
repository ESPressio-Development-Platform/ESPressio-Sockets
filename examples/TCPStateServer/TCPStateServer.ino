#include <WiFi.h>

#include <ESPressio_State.hpp>
#include <ESPressio_SocketState.hpp>

using namespace ESPressio;

struct DeviceTemperature {
    using Value = float;
    static constexpr State::StateTypeId Id = 0x54454D5045524154ULL;
};

struct OutputEnabled {
    using Value = bool;
    static constexpr State::StateTypeId Id = 0x4F5554505554454EULL;
};

using DeviceState = State::StateContract<DeviceTemperature, OutputEnabled>;

State::DeviceIdentifier localDevice =
    State::DeviceIdentifier::FromMacAddress(WiFi.macAddress().c_str()); // replace with your preferred stable device-ID construction

State::StatePublisher<DeviceState> publisher(localDevice);
State::RemoteStateManager<DeviceState, 4> remoteState;
State::StateSubscriptionRegistry<4> subscriptions;
Sockets::TCPStateServer<DeviceState, 4, 4> stateServer;

float temperature = 21.5f;
bool outputEnabled = false;

void setup() {
    Serial.begin(115200);

    WiFi.mode(WIFI_STA);
    WiFi.begin("YOUR_SSID", "YOUR_PASSWORD");
    while (WiFi.status() != WL_CONNECTED) delay(100);

    // In real code, construct a stable State::DeviceIdentifier for this device.
    uint8_t mac[6]{};
    WiFi.macAddress(mac);
    localDevice = State::DeviceIdentifier::FromMacAddress(mac);

    publisher.RegisterSource<DeviceTemperature>([] { return temperature; });
    publisher.RegisterSource<OutputEnabled>([] { return outputEnabled; });

    // Request these State definitions from every connected remote device.
    subscriptions.Subscribe<DeviceTemperature>();
    subscriptions.Subscribe<OutputEnabled>();

    Sockets::TCPStateServerConfig<DeviceState, 4, 4> config;
    config.Port = 2333;
    config.MaximumClients = 4;
    config.Session.MaximumProtocolMessageBytes = 512;

    if (!stateServer.Initialize(config, publisher, remoteState, subscriptions)) {
        Serial.println("Failed to start ESPressio TCP State server");
        return;
    }

    Serial.printf("State server listening on %s:%u\n", WiFi.localIP().toString().c_str(), config.Port);
}

void loop() {
    // When authoritative local data changes, publish the newest value. Every
    // connected peer that subscribed to that State definition receives it.
    static uint32_t lastPublication = 0;
    if (millis() - lastPublication >= 1000) {
        lastPublication = millis();
        publisher.Publish<DeviceTemperature>();
    }

    delay(10);
}
