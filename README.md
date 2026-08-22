# ESPressio Sockets

Socket-based ESPressio transports, Command adapters, transport-security sessions, and System Clock synchronization providers for the ESPressio Development Platform.

ESPressio Sockets concentrates **network I/O and socket framing** in one library while allowing Event semantics, Commands, clock discipline and cryptography to remain owned by the libraries responsible for those concepts.

## Current Version — 0.7.1

Sockets 0.7.1 is the repository-relocation dependency patch for the Sockets 0.7 generation. It validates the migrated ESPressio dependency stack while preserving the existing Socket Command protocol-v1 wire representation and modular opt-in Event, Command, Security, and Timing integrations.

The library provides reusable adapters around common IP/socket mechanisms without forcing application-level protocols to know the details of those mechanisms.

```text
Sockets 0.7.1
    -> Observable >= 3.0.2 < 4.0.0
```

## Dependencies

Required:

```text
ESPressio Observable >= 3.0.2 < 4.0.0
```

Optional integrations:

```text
Event >= 6.0.1 < 7.0.0
    socket Event Transport adapters
    Socket Event types
    SocketWorkerEventBridge
    SocketSecuritySessionEventBridge

Command >= 1.0.1 < 2.0.0
    SocketCommandSession / TCPCommandServer

Security >= 0.3.1 < 1.0.0
    SocketSecuritySession / SocketSecurityDatagram

Timing >= 2.2.5 < 3.0.0
    socket clock synchronization
```

External WebSocket/MQTT libraries remain associated only with the concrete adapters that use them.

See [ESPRESSIO_DEPENDENCY_CHART.md](ESPRESSIO_DEPENDENCY_CHART.md), [COMMAND_INTEGRATION.md](COMMAND_INTEGRATION.md), and [SECURITY_INTEGRATION.md](SECURITY_INTEGRATION.md).

## Installation

Core Sockets:

```ini
lib_deps =
    espressio-development-platform/ESPressio-Sockets@^0.7.1
    espressio-development-platform/ESPressio-Observable@^3.0.2

build_flags =
    -std=gnu++17
    -frtti

build_unflags =
    -std=gnu++11
    -fno-rtti
```

Add Event, Command, Security, Timing, WebSocket or MQTT dependencies only when selecting those integrations.

## Header structure and opt-in integrations

The normal umbrella is:

```cpp
#include <ESPressio_Sockets.hpp>
```

It deliberately does not pull in Event-, Command-, Security- or Timing-specific adapters.

This keeps a simple socket application from paying for integrations it does not use.

# Event Transports

Sockets supplies concrete ESPressio Event Transport adapters for socket-oriented mechanisms including:

```text
UDP
TCP client
TCP server
TLS
WebSocket client/server
MQTT
```

Event routing, Event identity, serialization and hop/origin semantics remain owned by ESPressio Event. Sockets only implements the concrete movement/framing required by the selected transport.

### Command 1.x structured values

Command 1.0.0 allows structured callers to retain native scalar `CommandValue` types. Socket Command protocol v1 intentionally remains wire-compatible with existing peers: typed values are normalized with `CommandValue::ToString()` when encoded, then reconstructed as string-backed `CommandValue` instances on decode. Registry parameter validation/conversion remains authoritative when the invocation executes.

This means existing protocol-v1 peers remain interoperable. Null `CommandValue` has no protocol-v1 representation and is rejected rather than silently encoded.

## Dependency direction

```text
Sockets Event integration - - -> Event
```

Event 6.0.0 does not depend back on Sockets, so the previous reciprocal Event/Sockets edge remains removed.

Sockets 0.6.0 owns the Event types and bridges representing its own Observable lifecycle:

```cpp
#include <ESPressio_SocketEvents.hpp>
#include <ESPressio_SocketWorkerEventBridge.hpp>
#include <ESPressio_SocketSecuritySessionEventBridge.hpp>
```

These header names remain familiar from their previous location in ESPressio Event, but the implementation now lives in the correct owning library.

# Command integration

`SocketCommandSession` and `TCPCommandServer` allow an ESPressio Command tree to be invoked over socket connections.

The integration supports both line-oriented and structured requests while keeping the authoritative command definition inside ESPressio Command.

Conceptually:

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
ESP-Now       0.8.0
Serial        0.7.2
```

For detailed remote Command usage, framing, policy hooks and examples, see [COMMAND_INTEGRATION.md](COMMAND_INTEGRATION.md).

See [ESPRESSIO_DEPENDENCY_CHART.md](ESPRESSIO_DEPENDENCY_CHART.md) and [CHANGELOG.md](CHANGELOG.md).

See [COMMAND_INTEGRATION.md](COMMAND_INTEGRATION.md) for the complete Command-over-Sockets contract.

# Clock synchronization

Optional ESPressio Timing integration provides socket mechanisms for carrying clock-synchronization exchanges while leaving offset estimation, filtering, stepping/slewing, drift learning and clock-discipline policy inside ESPressio Timing.

Supported socket-facing synchronization facilities include UDP/TCP/WebSocket mechanisms and external SNTP/NTP reference facilities where selected.

The important boundary is:

```text
Sockets
    captures/transports synchronization timestamps

Timing
    validates samples and disciplines SystemClock
```

Sockets therefore does not acquire its own competing clock discipline.

# Transport Security

Sockets provides adapters that bind stream/datagram semantics to ESPressio Security.

```text
Event / Command / application protocol
              |
              v
       TransportSecurity
              |
       +------+------+
       |             |
       v             v
SocketSecurity   SocketSecurity
Session          Datagram
       |             |
       v             v
stream socket    datagram socket
```

Sockets does not implement AES, ChaCha, keys, nonces, or replay algorithms. Those remain the responsibility of ESPressio Security.

## `SocketSecuritySession`

Use `SocketSecuritySession` with TCP/TLS/WebSocket-style byte streams:

```cpp
#include <ESPressio_SocketSecuritySession.hpp>

ESPressio::Sockets::SocketSecuritySession session(
    security,
    [&](const uint8_t* data, std::size_t size) {
        return client.write(data, size) == size;
    }
);
```

Each protected envelope is prefixed by a four-byte little-endian length. Incoming stream data can be supplied in whatever chunks the socket API produces:

```cpp
session.Feed(receivedData, receivedLength);
```

The session handles:

- one protected frame fragmented across multiple socket reads;
- multiple protected frames arriving in one read; and
- declared-size bounds through `SocketSecuritySessionConfig::MaximumProtectedFrameBytes`.

Only successfully authenticated/decrypted payloads are delivered upward.

## `SocketSecurityDatagram`

UDP and other message-oriented transports already preserve message boundaries, so one Security envelope can map directly to one datagram:

```cpp
#include <ESPressio_SocketSecurityDatagram.hpp>

ESPressio::Sockets::SocketSecurityDatagram datagram(
    security,
    sendDatagramCallback
);
```

Pass an incoming datagram to `Receive()`. It is delivered to the application only after authentication, decryption, protocol validation and replay validation succeed.

## TLS vs ESPressio Security

TLS and ESPressio Security protect different boundaries.

- TLS protects a connection/session.
- ESPressio Security protects an ESPressio application transport payload, including protocol binding, sender/session identity and replay semantics.

Applications may use Security over plaintext TCP/UDP, or combine it with TLS/WSS when defense-in-depth is appropriate.

## Securing Commands

The structured bytes representing a Command request can be routed through `SocketSecuritySession` before Command processing.

```text
socket bytes
    -> SocketSecuritySession
    -> authentication/decryption
    -> Command adapter
    -> CommandRegistry
```

Command therefore does not need its own direct Security dependency merely because a remote Command was transported securely.

# Observable lifecycle

Sockets exposes synchronous Observable lifecycle surfaces for the socket infrastructure itself.

`SocketWorker` reports meaningful start/start-failure/stop lifecycle changes, while `SocketSecuritySession` reports relevant secure-session fault/reset state changes.

These observations complement, rather than replace, the ordinary data/write/Command/Event/security callbacks that remain authoritative for their original responsibilities.

Applications that want asynchronous Event representations can opt into the Sockets-owned Event bridges described above.

# Failure handling

Security adapters expose Security failure information without exposing key material. Network-facing applications should distinguish authentication/replay/protocol failures from ordinary socket connection/I/O failures so diagnostics remain meaningful.

With Security policy `Required`, unauthenticated plaintext should never be treated as valid application traffic.

# Examples

The repository includes examples for Event transports, socket clock synchronization, TCP Command serving, and secure socket usage.

A representative secure TCP example is:

```text
examples/SecureTCPClient/SecureTCPClient.ino
```

Example credentials and key material are placeholders only and must not be treated as production provisioning guidance.

# Testing

Host and ESP32 validation exercise core socket facilities and the optional integrations, including fragmented/coalesced secure stream frames, declared frame-size limits, datagram protection, replay rejection, Command integration, synchronization protocols, lifecycle observation and Event bridge compilation.

# Compatibility and architectural guarantees

Sockets 0.6.0 preserves the modular integration model:

- socket Event Transport APIs remain opt-in;
- Command integration remains opt-in;
- Timing synchronization remains opt-in;
- Security integration remains opt-in;
- socket lifecycle Event bridges are now owned by Sockets;
- the normal umbrella remains independent of those optional domains.

The corrected dependency direction is:

```text
Sockets Event integration - - -> Event
Sockets Command integration - - -> Command
Sockets Security integration - - -> Security
Sockets Timing integration - - -> Timing
```

# Changelog

See [CHANGELOG.md](CHANGELOG.md) for release history and notable changes.
