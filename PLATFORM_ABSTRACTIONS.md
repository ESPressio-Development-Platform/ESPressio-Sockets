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
- Relocated the Arduino `UDPEventTransport`, `TCPClientEventTransport` and `TCPServerEventTransport` concrete implementations to ESPressio-ESP32.
- Their copies have been removed from ESPressio-Sockets, so this repository no longer owns the corresponding `WiFiUDP`, `WiFiClient` or `WiFiServer` implementations.
- Portable `IPv4Address`, `SocketEndpoint`, socket worker policy, framing and Event transport semantics remain owned by ESPressio-Sockets.
- TLS, WebSocket and MQTT concrete adapters still require a full target-integration audit. They must not force Arduino/ESP32 types back into shared public concepts.
- The package remains advertised as Arduino/ESP32 until those remaining concrete adapters are cleanly separated or conditionally packaged.

## Boundary rule

Socket protocols, framing, sessions and transport semantics belong in ESPressio-Sockets. Runtime execution belongs in System. Framework-specific network client/server implementations belong at the platform adapter boundary in ESPressio-ESP32 where they implement Sockets-owned protocol/domain contracts.
