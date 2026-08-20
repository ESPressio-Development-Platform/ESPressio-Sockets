# ESPressio Sockets

Socket-based ESPressio transports, Command adapters, transport-security sessions, and System Clock synchronization providers for the Flowduino ESPressio Development Platform.

## Latest Stable Version

The latest Stable Version is **0.4.0**.

## Current Development Version — 0.5.0

The `feature/observable-callback-coverage` branch targets **0.5.0** and adds native Observable lifecycle coverage while preserving the existing socket/data callback model.

For this development branch, the dependency model is:

```text
Required
    ESPressio Observable >= 3.0.1 < 4.0.0

Optional Command integration
    ESPressio Command >= 0.3.0 < 1.0.0

Optional Transport Security
    ESPressio Security >= 0.2.0 < 1.0.0

Optional Event integration
    ESPressio Event >= 5.8.0 < 6.0.0

Optional Timing synchronization
    ESPressio Timing >= 2.2.2 < 3.0.0
```

Observable coverage is owned by Sockets itself. `SocketWorker` exposes start/start-failure/stop lifecycle observation, and `SocketSecuritySession` exposes secure-session fault/reset observation. Existing receive, write, Command, Event Transport, and security-processing callbacks remain authoritative for their original responsibilities.

ESPressio Event remains **opt-in**. Event 5.8 provides `SocketWorkerEventBridge` and `SocketSecuritySessionEventBridge`; Sockets does not depend upward on Event. ESPressio Serial 0.5 can consume the same observer contracts directly for diagnostics without requiring Event.

The stable-release documentation below remains intact so existing 0.4.0 users retain accurate historical guidance.

## Compatibility

ESPressio Sockets `0.4.0` targets ESP32/Arduino-ESP32 and C++17. Individual facilities may depend on Arduino networking classes, WebSockets, MQTT, or optional ESPressio libraries according to the adapter selected.

## ESPressio Development Platform

ESPressio libraries are discrete, composable components with explicit responsibility boundaries. Sockets owns IP/socket transport mechanics; Event owns Event semantics, Command owns Command parsing/execution, Timing owns clock discipline, and Security owns encryption/authentication/replay protection.

## License

Apache License 2.0. See [LICENSE](LICENSE).

## ESPressio Library Dependencies

Core ESPressio Sockets has no mandatory ESPressio dependency in the stable 0.4.0 release. The 0.5.0 development branch adds the required Observable dependency documented above.

Optional integrations for stable 0.4.0:

```text
Event transports
    ESPressio Event >= 5.7.1 < 6.0.0

Command integration
    ESPressio Command >= 0.2.0 < 1.0.0

Clock synchronization
    ESPressio Timing >= 2.2.2 < 3.0.0

Transport Security
    ESPressio Security >= 0.1.0 < 1.0.0
```

External socket adapters continue to use WebSockets/PubSubClient where applicable.

See [ESPRESSIO_DEPENDENCY_CHART.md](ESPRESSIO_DEPENDENCY_CHART.md), [COMMAND_INTEGRATION.md](COMMAND_INTEGRATION.md), and [SECURITY_INTEGRATION.md](SECURITY_INTEGRATION.md).

## Namespace

```cpp
ESPressio::Sockets
```

## PlatformIO

Core Sockets:

```ini
lib_deps =
    https://github.com/Flowduino/ESPressio-Sockets@^0.4.0

build_flags =
    -std=gnu++17

build_unflags =
    -std=gnu++11
    -fno-rtti
```

Security integration:

```ini
lib_deps =
    https://github.com/Flowduino/ESPressio-Sockets@^0.4.0
    https://github.com/Flowduino/ESPressio-Security@^0.1.0
```

Add Event, Command, or Timing only when selecting those integrations.

For the 0.5.0 development branch, also include ESPressio Observable 3.0.1 or newer within the 3.x line, and use the Command/Security floors listed in the development-version section above.

## Header Structure

The normal umbrella is:

```cpp
#include <ESPressio_Sockets.hpp>
```

Dependency-bearing integrations are deliberately opt-in and are not included automatically.

Security headers:

```cpp
#include <ESPressio_SocketSecuritySession.hpp>
#include <ESPressio_SocketSecurityDatagram.hpp>
```

## Event Transports

ESPressio Sockets provides socket Event Transport adapters for:

```text
UDP
TCP client
TCP server
TLS
WebSocket client/server
MQTT
```

Event routing/type semantics remain owned by ESPressio Event rather than being embedded in Sockets.

## Command Integration

`SocketCommandSession` and `TCPCommandServer` allow ESPressio Command trees to be invoked over socket connections. The integration supports line-oriented and structured-binary requests, correlation IDs, per-client state, policy hooks, bounded request handling, metadata, result observation, and error handling.

See [COMMAND_INTEGRATION.md](COMMAND_INTEGRATION.md).

## Clock Synchronization

Optional Timing integration supplies UDP, TCP, and WebSocket synchronization mechanisms and external SNTP/NTP reference facilities while keeping clock-discipline policy inside ESPressio Timing.

## Transport Security

0.4.0 introduces optional ESPressio Security integration at the socket transport boundary.

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

Sockets does not implement AES, ChaCha, keys, nonces, or replay logic. It delegates those concerns to ESPressio Security and adapts socket framing/message boundaries.

### `SocketSecuritySession`

For TCP/TLS/WebSocket-style byte streams:

```cpp
Sockets::SocketSecuritySession session(
    security,
    [&](const uint8_t* data, std::size_t size) {
        return client.write(data, size) == size;
    }
);
```

Each protected envelope is prefixed by a four-byte little-endian length. `Feed()` accepts arbitrary stream chunks:

```cpp
session.Feed(receivedData, receivedLength);
```

It supports frames split across many reads and multiple frames arriving in one read. Declared frame lengths are bounded by `SocketSecuritySessionConfig::MaximumProtectedFrameBytes`.

### `SocketSecurityDatagram`

UDP/message-oriented sockets already preserve boundaries, so one ESPressio Security envelope is sent per datagram:

```cpp
Sockets::SocketSecurityDatagram datagram(
    security,
    sendDatagramCallback
);
```

The incoming datagram is passed to `Receive()` and is delivered upward only after Security authentication/decryption and replay validation succeed.

### Security Guarantees

The stable 0.4.0 adapters inherit Security 0.1.x semantics:

```text
AEAD encryption/authentication
protocol binding
key IDs and rotation
sender identity
authenticated session epoch
64-bit sequence numbers
sliding replay window
Disabled / Preferred / Required policies
```

`Required` is recommended for network-exposed Command/control traffic when plaintext must never be accepted.

## TLS vs ESPressio Security

TLS and ESPressio Security operate at different boundaries.

TLS protects a connection/session. ESPressio Security protects the application transport payload with ESPressio-specific protocol binding, sender/session identity and replay semantics.

Applications may use Security over plaintext TCP/UDP, or combine it with TLS/WSS as defense-in-depth.

## Securing Commands

The structured bytes used for Command invocation can be routed through `SocketSecuritySession`. Authentication/decryption therefore completes before the resulting Command invocation is passed to Command processing.

Command does not need a direct Security dependency.

The same architecture applies to Event or future application protocols.

## Failure Observation

Stable 0.4.0 Security adapters expose a failure callback carrying `SecurityResult`. This provides error classification for authentication, replay, key, algorithm, protocol, and frame-limit failures without exposing secret key material.

The 0.5.0 development branch additionally exposes the corresponding `ISocketSecuritySessionObserver` lifecycle contract and the general `ISocketWorkerObserver` lifecycle contract. These observer notifications complement rather than replace existing operational callbacks.

## Examples

The repository includes examples for Event transports, socket clock synchronization, TCP Command serving, and:

```text
examples/SecureTCPClient/SecureTCPClient.ino
```

The secure TCP example adapts `WiFiClient` to `SocketSecuritySession` using AES-256-GCM. Example credentials and key material are placeholders only.

## Testing

The host suite covers existing functionality and the new Security integration:

```text
CoreWithoutCommandOrSecurity
SocketCommand
SocketSecurity
ClockSynchronizationProtocol
```

`SocketSecurity` covers fragmented stream input, coalesced stream frames, declared-size limits, stream reset behavior, datagram protection and replay rejection. The 0.5.0 development branch extends host validation to the new observer lifecycle surface and tests against the refreshed Command/Security/Observable dependency generation.

## Compatibility

Sockets 0.4.0 is a backward-compatible minor release:

- existing Event Transport APIs remain unchanged;
- existing Command APIs remain unchanged;
- existing Timing synchronization remains unchanged;
- existing TLS/WSS behavior remains available;
- Security integration is opt-in;
- the normal umbrella remains independent of Security.

The 0.5.0 development branch is also designed as a backward-compatible minor extension. Observable becomes a core dependency because core worker lifecycle is now observable; Event itself remains optional.

## Contributing

Issues and contributions are welcome through GitHub. New socket mechanisms should keep application semantics and cryptography outside their concrete I/O responsibility wherever possible.

## Changelog

See [CHANGELOG.md](CHANGELOG.md).

## License

Apache License 2.0. See [LICENSE](LICENSE).
