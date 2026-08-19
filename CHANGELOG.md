# Changelog

## 0.2.0

- Added opt-in ESPressio Timing System Clock synchronization integration.
- Added shared socket Clock Synchronization wire protocol and configuration types.
- Added `UDPClockSynchronizer` with four-timestamp request/response synchronization.
- Added UDP authoritative broadcast synchronization.
- Added UDP authoritative multicast synchronization support.
- Added `TCPClockSynchronizationClient` and `TCPClockSynchronizationServer`.
- Added `WebSocketClockSynchronizationClient` and `WebSocketClockSynchronizationServer`.
- Added secure WebSocket client support for clock synchronization through the existing WebSocket dependency.
- Added `SNTPClockSyncProvider` using ESP-IDF/lwIP SNTP as an external Unix-time reference while preserving ESPressio Timing discipline and Observer notifications.
- Added `ESPressio_SocketClockSynchronization.hpp` opt-in batch header.
- Added UDP, TCP, WebSocket and SNTP synchronization examples.
- Preserved the existing Event Transport implementations unchanged.
- Kept ESPressio Timing optional for applications that do not include the Clock Synchronization headers.

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
