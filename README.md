# ESPressio Sockets

Socket-based ESPressio transports, Command adapters, Security sessions, and System Clock synchronization providers for the Flowduino ESPressio Development Platform.

## Current Version — 0.6.0

Sockets 0.6.0 owns the Event representations of its own socket-worker and socket-security-session lifecycle. The Sockets core remains modular: Event, Command, Security, and Timing integrations are selected explicitly.

## Core dependency

```text
Sockets 0.6.0
    -> Observable >= 3.0.1 < 4.0.0
```

External WebSocket/MQTT libraries remain associated with the concrete adapters that use them.

## Optional integrations

```text
Event >= 6.0.0 < 7.0.0
    socket Event Transport adapters
    Socket Event types
    SocketWorkerEventBridge
    SocketSecuritySessionEventBridge

Command >= 0.4.0 < 1.0.0
    SocketCommandSession / TCPCommandServer

Security >= 0.3.0 < 1.0.0
    SocketSecuritySession / SocketSecurityDatagram

Timing >= 2.2.4 < 3.0.0
    socket clock synchronization
```

The public Socket Event bridge/header names are preserved from their former location in ESPressio Event, but ownership now matches the Sockets domain.

## Dependency direction

```text
Sockets Event integration - - -> Event
Sockets Command integration - - -> Command
Sockets Security integration - - -> Security
Sockets Timing integration - - -> Timing
```

Event 6.0.0 does not depend back on Sockets, so the previous reciprocal Event/Sockets edge is removed.

The normal umbrella:

```cpp
#include <ESPressio_Sockets.hpp>
```

does not include Event-, Command-, Security-, or Timing-specific adapters.

## Final coordinated generation

```text
Observable    3.0.1
Serializable  0.10.2
Units         0.2.3
Timing        2.2.4
Threads       3.1.4
Command       0.4.0
Security      0.3.0
Event         6.0.0
Sockets       0.6.0
ESP-Now       0.6.0
Serial        0.6.0
```

See [ESPRESSIO_DEPENDENCY_CHART.md](ESPRESSIO_DEPENDENCY_CHART.md) and [CHANGELOG.md](CHANGELOG.md).

## License

Apache License 2.0. See [LICENSE](LICENSE).
