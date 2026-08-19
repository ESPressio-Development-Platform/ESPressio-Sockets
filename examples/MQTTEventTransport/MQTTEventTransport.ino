#include <Arduino.h>
#include <WiFi.h>

#include <ESPressio_EventTransport.hpp>
#include <ESPressio_MQTTEventTransport.hpp>

using namespace ESPressio;

const char* SSID = "YOUR_WIFI";
const char* PASSWORD = "YOUR_PASSWORD";

class MQTTDemoEvent :
    public Event::Event<>,
    public Serializable::
        SerializableBase<
            MQTTDemoEvent
        > {

public:
    uint32_t Sequence = 0;

    ESPRESSIO_SERIALIZABLE_TYPE(
        MQTTDemoEvent
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
    MQTTDemoEvent,
    "flowduino.example.sockets.mqtt-demo.v1"
)

Sockets::MQTTEventTransport mqttTransport;

void setup() {
    Serial.begin(115200);

    WiFi.begin(SSID, PASSWORD);

    while (
        WiFi.status() != WL_CONNECTED
    ) {
        delay(250);
    }

    Sockets::MQTTEventTransportConfig
        config;

    config.Host =
        "192.168.1.10";

    config.Port = 1883;

    /*
     * Use a unique MQTT Client ID on every device.
     */
    config.ClientID =
        "espressio-device-a";

    /*
     * For two devices, swap these topics on the second device.
     *
     * Device A:
     *   outbound = espressio/a-to-b
     *   inbound  = espressio/b-to-a
     *
     * Device B:
     *   outbound = espressio/b-to-a
     *   inbound  = espressio/a-to-b
     */
    config.OutboundTopic =
        "espressio/a-to-b";

    config.InboundTopic =
        "espressio/b-to-a";

    config.BufferSize = 4096;

    mqttTransport.Initialize(
        config
    );

    auto& manager =
        Event::EventTransportManager::
            GetInstance();

    manager.RegisterTransport(
        &mqttTransport
    );

    manager.RegisterBidirectionalEvent<
        MQTTDemoEvent
    >(
        &mqttTransport
    );

    manager.Initialize();
}

void loop() {
    static uint32_t sequence = 0;
    static uint32_t lastSend = 0;

    if (
        millis() - lastSend >=
        5000
    ) {
        lastSend = millis();

        auto* event =
            new MQTTDemoEvent();

        event->Sequence =
            ++sequence;

        event->Queue();
    }

    delay(10);
}
