# Platform Abstractions Audit Trail

This file records Sockets changes made during the platform-abstraction tranche tracked by issue #32.

## 2026-08-27

### Working branch
- Created `feature/32-platform-network-abstractions` from `main` before making tranche changes.

### Portable public concepts
- Replaced Arduino `IPAddress` in `SocketEndpoint` with an ESPressio-owned `IPv4Address` value type.
- Replaced FreeRTOS priority/core types in `SocketWorkerConfig` with standard integer priority and `System::ProcessorAffinity`.
- Public socket endpoint/configuration concepts no longer require Arduino or FreeRTOS headers.

### Worker runtime
- Replaced direct FreeRTOS task creation, delay, yield and current-task checks in `SocketWorker` with `ESPressio::System::Execution`.
- Socket transports continue to own their network-domain worker policy; the execution mechanism is now supplied by the installed platform provider.

### Concrete network adapters
- Relocated Arduino `UDPEventTransport`, `TCPClientEventTransport`, `TCPServerEventTransport`, `TLSEventTransport`, `WebSocketClientEventTransport`, `WebSocketServerEventTransport` and `MQTTEventTransport` implementations to ESPressio-ESP32.
- Their copies have been removed from ESPressio-Sockets, so this repository no longer owns `WiFiUDP`, `WiFiClient`, `WiFiServer`, `WiFiClientSecure`, Links2004 WebSocket or PubSubClient implementations.
- Relocated TCP/TLS/MQTT reconnect timing uses the System monotonic clock rather than Arduino `millis()`.
- Relocated TCP/TLS/WebSocket/MQTT configuration uses `std::string` rather than Arduino `String`.
- Portable `IPv4Address`, `SocketEndpoint`, socket worker policy, framing, sessions, Command/Security/Timing integration and Event transport semantics remain owned by ESPressio-Sockets.

### Package boundary
- ESPressio-Sockets now advertises `frameworks: *` and `platforms: *`.
- Links2004 WebSockets and PubSubClient are no longer dependencies of the portable Sockets package; ESPressio-ESP32 declares them because its concrete adapters consume them.

## Boundary rule

Socket protocols, framing, sessions and transport semantics belong in ESPressio-Sockets. Runtime execution belongs in System. Framework-specific network client/server implementations belong at the platform adapter boundary in ESPressio-ESP32 where they implement Sockets-owned protocol/domain contracts.
