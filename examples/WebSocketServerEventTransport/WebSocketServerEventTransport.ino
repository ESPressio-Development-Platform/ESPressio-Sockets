#include <Arduino.h>
#include <WiFi.h>

#include <ESPressio_EventTransport.hpp>
#include <ESPressio_WebSocketServerEventTransport.hpp>

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


Sockets::WebSocketServerEventTransport webSocketServer;

void setup() {
    Serial.begin(115200);

    WiFi.begin(SSID, PASSWORD);
    while (WiFi.status() != WL_CONNECTED) {
        delay(250);
    }

    Serial.print("WebSocket Event server: ws://");
    Serial.print(WiFi.localIP());
    Serial.println(":44000/");

    Sockets::WebSocketServerEventTransportConfig config;
    config.Port = 44000;
    config.Protocol = "espressio";

    webSocketServer.Initialize(config);

    auto& manager =
        Event::EventTransportManager::
            GetInstance();

    manager.RegisterTransport(
        &webSocketServer
    );

    manager.RegisterBidirectionalEvent<
        SocketDemoEvent
    >(
        &webSocketServer
    );

    manager.Initialize();
}

void loop() {
    delay(1000);
}
