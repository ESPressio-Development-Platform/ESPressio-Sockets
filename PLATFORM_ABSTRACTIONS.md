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
- `UDPEventTransport` now keeps conversion between portable `IPv4Address` and Arduino `IPAddress` local to the concrete WiFiUDP adapter.
- TCP, TLS, WebSocket and MQTT concrete adapters still require a full target-integration audit. They must not force Arduino/ESP32 types back into shared public concepts.
- The package remains advertised as Arduino/ESP32 until those concrete adapters are cleanly separated or conditionally packaged.

## Boundary rule

Socket protocols, framing, sessions and transport semantics belong in ESPressio-Sockets. Runtime execution belongs in System. Framework-specific network client/server implementations must stay at a clearly isolated concrete-adapter boundary and may ultimately move to ESPressio-ESP32 where that provides the cleaner ownership model.
