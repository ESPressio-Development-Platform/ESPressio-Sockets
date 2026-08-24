# ESPressio Sockets

Socket-based ESPressio transports, Command adapters, transport-security sessions, and System Clock synchronization providers for the ESPressio Development Platform.

ESPressio Sockets concentrates **network I/O and socket framing** in one library while allowing Event semantics, Commands, clock discipline and cryptography to remain owned by the libraries responsible for those concepts.

## Current Version — 0.7.3

Sockets 0.7.3 is a dependency-maintenance release aligning all optional ESPressio integration surfaces with the Serializable 0.11.3 cascade while preserving the existing Socket Command protocol-v1 wire representation and modular integration architecture.

```text
Sockets 0.7.3
    -> Observable >= 3.0.2 < 4.0.0
```

## Dependencies

Required:

```text
ESPressio Observable >= 3.0.2 < 4.0.0
```

Optional integrations:

```text
Event >= 6.0.3 < 7.0.0
Command >= 1.0.3 < 2.0.0
Security >= 0.4.2 < 1.0.0
Timing >= 2.2.8 < 3.0.0
```

External WebSocket/MQTT libraries remain associated only with the concrete adapters that use them.

See [ESPRESSIO_DEPENDENCY_CHART.md](ESPRESSIO_DEPENDENCY_CHART.md), [COMMAND_INTEGRATION.md](COMMAND_INTEGRATION.md), and [SECURITY_INTEGRATION.md](SECURITY_INTEGRATION.md).

## Installation

```ini
lib_deps =
    espressio-development-platform/ESPressio-Sockets@^0.7.3
    espressio-development-platform/ESPressio-Observable@^3.0.2
```

Add Event, Command, Security, Timing, WebSocket or MQTT dependencies only when selecting those integrations.

## Header structure and opt-in integrations

The normal umbrella is:

```cpp
#include <ESPressio_Sockets.hpp>
```

It deliberately does not pull in Event-, Command-, Security- or Timing-specific adapters. This keeps a simple socket application from paying for integrations it does not use.

# Event Transports

Sockets supplies concrete ESPressio Event Transport adapters for UDP, TCP client/server, TLS, WebSocket and MQTT mechanisms. Event routing, Event identity, serialization and hop/origin semantics remain owned by ESPressio Event.

Sockets owns the Event types and bridges representing its own Observable lifecycle:

```cpp
#include <ESPressio_SocketEvents.hpp>
#include <ESPressio_SocketWorkerEventBridge.hpp>
#include <ESPressio_SocketSecuritySessionEventBridge.hpp>
```

# Command integration

`SocketCommandSession` and `TCPCommandServer` allow an ESPressio Command tree to be invoked over socket connections. Command integration remains opt-in and is validated against Command 1.0.3.

Socket Command protocol v1 remains wire-compatible: native scalar `CommandValue` values are normalized with `ToString()` at the existing wire boundary and decoded as string-backed values. Null has no protocol-v1 representation and remains rejected.

# Clock synchronization

Optional Timing integration carries clock-synchronization exchanges while Timing remains responsible for sample validation, estimation and SystemClock discipline. Sockets 0.7.3 validates this integration against Timing 2.2.8.

# Transport Security

`SocketSecuritySession` and `SocketSecurityDatagram` bind stream/datagram semantics to ESPressio Security without implementing cryptography themselves. Security integration is validated against Security 0.4.2.

TLS and ESPressio Security continue to protect different boundaries: TLS protects a connection/session; ESPressio Security protects application transport payloads with protocol binding, sender/session identity and replay semantics.

# Observable lifecycle

Sockets exposes synchronous Observable lifecycle surfaces for socket infrastructure. Applications requiring asynchronous Event representations can opt into the Sockets-owned Event bridges.

# Serializable 0.11.3 cascade generation

```text
Observable    3.0.2
Serializable  0.11.3
Units         0.2.7
Timing        2.2.8
Threads       3.1.7
Event         6.0.3
Command       1.0.3
Security      0.4.2
Persistence   0.3.2
Sockets       0.7.3
```

# Compatibility and architectural guarantees

- socket Event Transport APIs remain opt-in;
- Command integration remains opt-in;
- Timing synchronization remains opt-in;
- Security integration remains opt-in;
- the normal umbrella remains independent of those optional domains;
- Socket Command protocol-v1 wire format is unchanged;
- no Sockets runtime behaviour changes are introduced by 0.7.3.

The dependency direction remains:

```text
Sockets Event integration - - -> Event
Sockets Command integration - - -> Command
Sockets Security integration - - -> Security
Sockets Timing integration - - -> Timing
```

# Testing

Host and ESP32 validation exercise core socket facilities and optional integrations against the released Serializable 0.11.3 cascade, including secure stream/datagram handling, Command integration, synchronization protocols, lifecycle observation and Event bridge compilation.

# Changelog

See [CHANGELOG.md](CHANGELOG.md) for release history and notable changes.
