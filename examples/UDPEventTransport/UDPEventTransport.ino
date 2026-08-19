#include <Arduino.h>
#include <WiFi.h>

#include <ESPressio_EventTransport.hpp>
#include <ESPressio_UDPEventTransport.hpp>

using namespace ESPressio;

const char* SSID = "YOUR_WIFI";
const char* PASSWORD = "YOUR_PASSWORD";


class SocketDemoEvent :
    public Event::Event<>,
    public Serializable::
        SerializableBase<
            SocketDemoEvent
        > {

public:
    uint32_t Sequence = 0;

    ESPRESSIO_SERIALIZABLE_TYPE(
        SocketDemoEvent
    )

    ESPRESSIO_SERIALIZABLE_SCHEMA_VERSION(1)

    ESPRESSIO_SERIALIZABLE_PROPERTIES(
        ESPRESSIO_PROPERTY(
            "sequence",
            Sequence
        )
    )
};

ESPRESSIO_EVENT_TRANSPORT_TYPE(
    SocketDemoEvent,
    "flowduino.example.sockets.demo.v1"
)


Sockets::UDPEventTransport udpTransport;

void setup() {
    Serial.begin(115200);

    WiFi.begin(SSID, PASSWORD);
    while (WiFi.status() != WL_CONNECTED) {
        delay(250);
    }

    Sockets::UDPEventTransportConfig config;
    config.LocalPort = 42000;

    udpTransport.Initialize(config);

    // Broadcast to every device listening on UDP port 42000.
    udpTransport.AddBroadcastDestination(42000);

    auto& manager =
        Event::EventTransportManager::
            GetInstance();

    manager.RegisterTransport(
        &udpTransport
    );

    manager.RegisterBidirectionalEvent<
        SocketDemoEvent
    >(
        &udpTransport
    );

    manager.Initialize();
}

void loop() {
    static uint32_t sequence = 0;
    static uint32_t lastSend = 0;

    if (millis() - lastSend >= 5000) {
        lastSend = millis();

        auto* event =
            new SocketDemoEvent();

        event->Sequence =
            ++sequence;

        event->Queue();
    }

    delay(10);
}
