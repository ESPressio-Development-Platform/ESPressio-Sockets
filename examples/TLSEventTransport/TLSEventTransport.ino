#include <Arduino.h>
#include <WiFi.h>

#include <ESPressio_EventTransport.hpp>
#include <ESPressio_TLSEventTransport.hpp>

using namespace ESPressio;

const char* SSID = "YOUR_WIFI";
const char* PASSWORD = "YOUR_PASSWORD";


/**
 * ESPressio Memory Audit
 * Inherited Memory Total: 4 bytes known bases + sizeof(std::atomic_flag) [0 bytes dynamic allocation]
 * Members:
 * - Sequence (uint32_t): 4 bytes [0 bytes dynamic allocation]
 * Total Memory: 4 bytes known bases + sizeof(std::atomic_flag) + 4 bytes known members [0 bytes dynamic allocation]
 * Basis: ESP32/Xtensa ILP32 reference ABI; GNU libstdc++ container control-block sizes are implementation-sensitive.
 * Confidence: low; compile-time sizeof on the concrete target remains authoritative for ABI-sensitive/opaque members.
 * End ESPressio Memory Audit
 */
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


// Replace with the CA certificate used by your TLS Event endpoint.
static const char ROOT_CA[] PROGMEM = R"EOF(
-----BEGIN CERTIFICATE-----
YOUR_CA_CERTIFICATE
-----END CERTIFICATE-----
)EOF";

Sockets::TLSEventTransport tlsTransport;

void setup() {
    Serial.begin(115200);

    WiFi.begin(SSID, PASSWORD);
    while (WiFi.status() != WL_CONNECTED) {
        delay(250);
    }

    Sockets::TLSEventTransportConfig config;
    config.Host = "event-server.example.com";
    config.Port = 4433;
    config.CACertificate = ROOT_CA;

    tlsTransport.Initialize(config);

    auto& manager =
        Event::EventTransportManager::
            GetInstance();

    manager.RegisterTransport(&tlsTransport);

    manager.RegisterBidirectionalEvent<
        SocketDemoEvent
    >(
        &tlsTransport
    );

    manager.Initialize();
}

void loop() {
    delay(1000);
}
