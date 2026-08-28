# ESPressio Sockets

Socket-oriented ESPressio transports, framing, Command adapters, State sessions, transport-security sessions, and System Clock synchronization providers for the ESPressio Development Platform.

ESPressio Sockets owns **generic non-Web socket concerns** such as TCP, UDP and socket-oriented framing. Web protocols are owned by ESPressio-Web. Event semantics, Commands, State semantics, clock discipline and cryptography remain owned by their respective domain libraries.

## Active working-branch architecture

The active `feature/state-transport-major-release` branch includes transport-neutral State integration and the platform-abstraction work staged for the later coordinated release restructuring. The package version remains `0.7.3` during this development tranche; version changes are intentionally deferred.

Core Sockets remains portable and depends only on ESPressio-System and ESPressio-Observable. Event, Command, State, Timing and Security integrations remain opt-in and are kept out of the normal `ESPressio_Sockets.hpp` umbrella unless their specific integration headers are included.

WebSocket ownership has moved to ESPressio-Web. Sockets no longer owns WebSocket clients/servers, WebSocket Event transports, WebSocket clock-synchronization wrappers, WebSocket routes, or WebSocket platform dependencies. Reusable transport-neutral protocol/session machinery may still be consumed by ESPressio-Web where appropriate.

## Current package version

```text
ESPressio-Sockets 0.7.3
```

The version is deliberately unchanged during the current platform restructuring.

## Dependencies

Required by the active working branch:

```text
ESPressio System
ESPressio Observable
```

Optional integrations include:

```text
Event
Command
State
Security
Timing
```

Concrete platform network adapters are supplied by architecture packages such as ESPressio-ESP32. WebSocket implementations and WebSocket-specific primitive adapters are supplied through ESPressio-Web and its platform implementation.

See [ESPRESSIO_DEPENDENCY_CHART.md](ESPRESSIO_DEPENDENCY_CHART.md), [COMMAND_INTEGRATION.md](COMMAND_INTEGRATION.md), and [SECURITY_INTEGRATION.md](SECURITY_INTEGRATION.md).

## Header structure and opt-in integrations

The normal umbrella is:

```cpp
#include <ESPressio_Sockets.hpp>
```

It deliberately does not pull in Event-, Command-, State-, Security- or Timing-specific adapters.

## State integration

`SocketStateSession` is a transport-neutral State session layered over a caller-supplied byte transport. Sockets owns framing and connection/session adaptation; ESPressio-State owns contracts, publishers, subscriptions, remote repositories, epochs/revisions, acknowledgement semantics and State protocol meaning.

A session binds an authoritative `StatePublisher`, a `RemoteStateManager`, and a `StateSubscriptionRegistry` to a send callback. Received stream bytes are passed to `session.Feed(...)`; framing supports fragmented stream delivery and rejects messages exceeding configured bounds.

The integration preserves State ownership: local subscriptions are propagated, accepted subscriptions can yield immediate authoritative snapshots, later publications update remote repositories, acknowledgements preserve epoch/revision semantics, and teardown does not introduce a reverse dependency from State to Sockets.

## Event transports

Socket-oriented ESPressio Event transport adapters cover mechanisms owned by the socket/platform layer, including UDP, TCP and TLS. MQTT remains a separate concrete network adapter where enabled.

WebSocket Event transport is **not** owned by Sockets; it is provided by ESPressio-Web and binds to an application-published WebSocket endpoint/client.

Event routing, identity, serialization and hop/origin semantics remain owned by ESPressio-Event.

Sockets-owned Observable lifecycle bridges remain available through their specific integration headers.

## Command integration

`SocketCommandSession` and socket server adapters allow an ESPressio Command tree to be invoked over byte-oriented socket connections. `SocketCommandSession` intentionally remains transport-neutral: inbound bytes are supplied through `Feed(...)` and outbound bytes through a caller-provided writer.

This makes the session reusable by ESPressio-Web without making Sockets own WebSocket protocol or routing semantics.

## Clock synchronization

Optional Timing integration carries clock-synchronization exchanges while Timing remains responsible for sample validation, estimation and System Clock discipline.

`SocketClockSynchronizationProtocol` remains in Sockets because its binary exchange protocol is reusable across socket transports and Web adapters. Its portable configuration/types are separated from UDP-specific `IPAddress` configuration so consumers such as ESPressio-Web do not inherit platform-specific UDP types.

TCP and UDP synchronization providers remain in Sockets. WebSocket clock synchronization is provided by ESPressio-Web and reuses the portable protocol.

## Transport Security

`SocketSecuritySession` and `SocketSecurityDatagram` bind stream/datagram semantics to ESPressio-Security without implementing cryptography themselves.

TLS and ESPressio-Security protect different boundaries: TLS protects a connection/session; ESPressio-Security protects application transport payloads with protocol binding, sender/session identity and replay semantics.

## Observable lifecycle

Sockets exposes synchronous Observable lifecycle surfaces for socket infrastructure. Applications requiring asynchronous Event representations can opt into the Sockets-owned Event bridges.

## Architectural guarantees

- core Sockets remains portable;
- Sockets owns generic non-Web socket concerns, not Web protocols;
- WebSocket ownership resides in ESPressio-Web;
- Event, Command, State, Timing and Security integrations remain opt-in;
- the normal umbrella remains independent of optional domain integrations;
- State retains ownership of State-domain protocol/revision/subscription semantics;
- target-specific network implementation is supplied through ESPressio platform providers rather than reusable Sockets code;
- no backwards-compatibility forwarding header is retained for concrete socket Event transports during this forward-moving development tranche.

The dependency direction remains:

```text
Sockets Event integration - - -> Event
Sockets Command integration - - -> Command
Sockets State integration - - -> State
Sockets Security integration - - -> Security
Sockets Timing integration - - -> Timing

Web may consume transport-neutral Sockets protocol/session machinery
without transferring WebSocket ownership back to Sockets.
```

## Testing

Host and target validation exercise core socket facilities and optional integrations. WebSocket-specific validation is now the responsibility of ESPressio-Web and the appropriate concrete platform package.

## Changelog

See [CHANGELOG.md](CHANGELOG.md) for release history and notable changes.
