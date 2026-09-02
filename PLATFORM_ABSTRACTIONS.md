# Platform Abstractions Audit Trail

This file records Sockets changes made during the platform-abstraction tranche, including the platform-boundary work tracked by issue #32 and State transport integration work tracked by issue #30.

## 2026-08-27

### Working branch
- The authoritative active working branch is `feature/state-transport-major-release`.
- An intermediate #32 branch was identified as having been based on `main` instead of this active branch. No branch histories were merged or consolidated: the #32 result was replayed directly onto `feature/state-transport-major-release`, preserving the existing State transport work, and all subsequent work/validation has continued on this active branch.
- Rollback branches remain untouched.

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

### State transport integration
- State-over-Sockets remains an opt-in Sockets-owned domain integration; ESPressio-State does not depend on Sockets.
- The active State integration is validated against ESPressio-State `feature/1-state-foundation`.
- A synchronous re-entrancy failure was isolated in `SocketStateFrameDecoder`: the decoder previously invoked the frame callback while the current frame was still present at the front of its internal receive buffer. A synchronous callback that fed another frame into the same decoder could therefore decode the outer frame again recursively.
- The decoder now consumes the current frame before invoking downstream/session code and keeps the callback payload in independent storage so nested `Push()` calls cannot reprocess the outer frame or invalidate its payload.
- Added a direct nested-`Push()` regression in addition to the full two-session State integration test.
- The State integration workflow now uses a bounded execution timeout so future synchronous protocol deadlocks/recursion fail deterministically rather than occupying a CI runner indefinitely.

## Boundary rule

Socket protocols, framing, sessions and transport semantics belong in ESPressio-Sockets. Runtime execution belongs in System. Framework-specific network client/server implementations belong at the platform adapter boundary in ESPressio-ESP32 where they implement Sockets-owned protocol/domain contracts.
