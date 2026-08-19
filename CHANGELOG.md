# Changelog

## 0.1.0

Initial ESPressio Sockets release.

- Added common socket endpoint and worker configuration types.
- Added versioned TCP/TLS Event stream framing.
- Added `UDPEventTransport`.
- Added UDP unicast, broadcast and multicast support.
- Added `TCPClientEventTransport`.
- Added `TCPServerEventTransport` with multi-client fan-out.
- Added `TLSEventTransport` using Arduino-ESP32 `WiFiClientSecure`.
- Added `WebSocketClientEventTransport` with `ws` and `wss` support.
- Added `WebSocketServerEventTransport`.
- Added `MQTTEventTransport` with plain MQTT and MQTT-over-TLS support.
- Added PubSubClient 2.8 as the supported MQTT implementation.
- Added Links2004 WebSockets 2.3.6 as the supported WebSocket implementation.
- Added per-protocol examples.
- Targeted C++17 and ESPressio Event 5.4 Event Transport semantics.
