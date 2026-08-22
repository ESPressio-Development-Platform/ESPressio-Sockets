# ESPressio Sockets

Socket-based ESPressio transports, Command adapters, Security sessions, and System Clock synchronization providers for the Flowduino ESPressio Development Platform.

## Current Version — 0.7.0

Sockets 0.7.0 adds compatibility with ESPressio Command 1.0.0's typed `CommandInvocation` values while preserving the existing Socket Command protocol-v1 wire representation. The Sockets core remains modular: Event, Command, Security, and Timing integrations are selected explicitly.

## Core dependency

```text
Sockets 0.7.0
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

Command >= 1.0.0 < 2.0.0
    SocketCommandSession / TCPCommandServer

Security >= 0.3.0 < 1.0.0
    SocketSecuritySession / SocketSecurityDatagram

Timing >= 2.2.4 < 3.0.0
    socket clock synchronization
```

The public Socket Event bridge/header names are preserved from their former location in ESPressio Event, but ownership now matches the Sockets domain.

### Command 1.x structured values

Command 1.0.0 allows structured callers to retain native scalar `CommandValue` types. Socket Command protocol v1 intentionally remains wire-compatible with existing peers: typed values are normalized with `CommandValue::ToString()` when encoded, then reconstructed as string-backed `CommandValue` instances on decode. Registry parameter validation/conversion remains authoritative when the invocation executes.

This means existing protocol-v1 peers remain interoperable. Null `CommandValue` has no protocol-v1 representation and is rejected rather than silently encoded.

## Dependency direction

```text
Sockets Event integration - - -> Event
Sockets Command integration - - -> Command
Sockets Security integration - - -> Security
Sockets Timing integration - - -> Timing
```

Event 6.0.0 does not depend back on Sockets, so the previous reciprocal Event/Sockets edge remains removed.

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
Command       1.0.0
Security      0.3.0
Event         6.0.0
Sockets       0.7.0
ESP-Now       0.7.0
Serial        0.7.0
```

For detailed remote Command usage, framing, policy hooks and examples, see [COMMAND_INTEGRATION.md](COMMAND_INTEGRATION.md).

See [ESPRESSIO_DEPENDENCY_CHART.md](ESPRESSIO_DEPENDENCY_CHART.md) and [CHANGELOG.md](CHANGELOG.md).

## License

Apache License 2.0. See [LICENSE](LICENSE).
