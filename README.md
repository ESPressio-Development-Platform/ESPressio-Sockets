# ESPressio Sockets

Socket-based ESPressio transports, Command adapters, transport-security sessions, State sessions, and System Clock synchronization providers for the ESPressio Development Platform.

ESPressio Sockets concentrates **network I/O and socket framing** in one library while allowing Event semantics, Commands, State semantics, clock discipline and cryptography to remain owned by the libraries responsible for those concepts.

## Active working-branch architecture

The active `feature/state-transport-major-release` branch adds transport-neutral ESPressio State integration and the platform-abstraction work staged for the next major Sockets release. The published `0.7.3` guidance below is retained as release-history documentation until the coordinated `1.0.0` release preparation is completed.

Core Sockets remains portable and depends only on ESPressio-System and ESPressio-Observable. Event, Command, State, Timing and Security integrations remain opt-in and are kept out of the normal `ESPressio_Sockets.hpp` umbrella unless their specific integration headers are included.

## Current Version — 0.7.3

Sockets 0.7.3 is a dependency-maintenance release aligning all optional ESPressio integration surfaces with the Serializable 0.11.3 cascade while preserving the existing Socket Command protocol-v1 wire representation and modular integration architecture.

```text
Sockets 0.7.3
    -> Observable >= 3.0.2 < 4.0.0
```

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

For coordinated development, those integrations resolve their mandatory ESPressio dependencies from the corresponding active working branches. Published `0.7.3` applications should continue to use the released dependency ranges appropriate to that release.

External WebSocket/MQTT libraries remain associated only with the concrete adapters that use them.

See [ESPRESSIO_DEPENDENCY_CHART.md](ESPRESSIO_DEPENDENCY_CHART.md), [COMMAND_INTEGRATION.md](COMMAND_INTEGRATION.md), and [SECURITY_INTEGRATION.md](SECURITY_INTEGRATION.md).

## Installation

Published 0.7.3:

```ini
lib_deps =
    espressio-development-platform/ESPressio-Sockets@^0.7.3
    espressio-development-platform/ESPressio-Observable@^3.0.2
```

Add Event, Command, State, Security, Timing, WebSocket or MQTT dependencies only when selecting those integrations.

## Header structure and opt-in integrations

The normal umbrella is:

```cpp
#include <ESPressio_Sockets.hpp>
```

It deliberately does not pull in Event-, Command-, State-, Security- or Timing-specific adapters. This keeps a simple socket application from paying for integrations it does not use.

# State integration

The active working branch provides `SocketStateSession`, a transport-neutral State session layered over a caller-supplied byte transport. Sockets owns framing and connection/session adaptation; ESPressio-State continues to own contracts, publishers, subscriptions, remote repositories, epochs/revisions, acknowledgement semantics and State protocol meaning.

A session binds an authoritative `StatePublisher`, a `RemoteStateManager`, and a `StateSubscriptionRegistry` to a send callback:

```cpp
#include <ESPressio_State.hpp>
#include <ESPressio_SocketStateSession.hpp>

using Contract = ESPressio::State::StateContract<MyState, OtherState>;
using Publisher = ESPressio::State::StatePublisher<Contract>;
using Remote = ESPressio::State::RemoteStateManager<Contract, 4>;
using Subscriptions = ESPressio::State::StateSubscriptionRegistry<8>;
using Session = ESPressio::Sockets::SocketStateSession<Contract, 4, 8>;

Publisher publisher(localDevice);
Remote remote;
Subscriptions subscriptions;
Session session;

ESPressio::Sockets::SocketStateSessionConfig config;
config.MaximumProtocolMessageBytes = 512;

session.Initialize(
    publisher,
    remote,
    subscriptions,
    config,
    [&](const uint8_t* data, std::size_t size) {
        return SendBytesToPeer(data, size);
    }
);
```

Received stream bytes are passed to `session.Feed(...)`. The framing layer supports fragmented stream delivery and rejects messages exceeding the configured bound.

The integration preserves State's transport-neutral behavior:

- local subscriptions are propagated to the peer;
- accepted subscriptions can produce an immediate authoritative snapshot;
- later publications update the remote repository;
- acknowledgements preserve State epoch/revision semantics;
- unsubscribe prevents later values from mutating the corresponding remote subscription state;
- connection/session teardown does not create a reverse dependency from State to Sockets.

Subscription-registry Observer callbacks may synchronously trigger transport activity and feed data back into State. Consequently, ESPressio-State dispatches subscription/unsubscription notifications only after releasing its registry mutex. State issue #12 includes a regression that re-enters `IsSubscribed()` and `Count()` from those callbacks; Sockets' host State-session integration exercises the same synchronous round-trip path.

The active host integration test also validates bidirectional subscriptions/publications and fragmented frame decoding.

# Event Transports

Sockets supplies concrete ESPressio Event Transport adapters for UDP, TCP client/server, TLS, WebSocket and MQTT mechanisms. Event routing, Event identity, serialization and hop/origin semantics remain owned by ESPressio Event.

Sockets owns the Event types and bridges representing its own Observable lifecycle:

```cpp
#include <ESPressio_SocketEvents.hpp>
#include <ESPressio_SocketWorkerEventBridge.hpp>
#include <ESPressio_SocketSecuritySessionEventBridge.hpp>
```

# Command integration

`SocketCommandSession` and `TCPCommandServer` allow an ESPressio Command tree to be invoked over socket connections. Command integration remains opt-in and published 0.7.3 is validated against its corresponding released Command baseline.

Socket Command protocol v1 remains wire-compatible: native scalar `CommandValue` values are normalized with `ToString()` at the existing wire boundary and decoded as string-backed values. Null has no protocol-v1 representation and remains rejected.

# Clock synchronization

Optional Timing integration carries clock-synchronization exchanges while Timing remains responsible for sample validation, estimation and SystemClock discipline. The active branch consumes the coordinated Timing platform-clock abstraction work; published 0.7.3 retains its released Timing baseline.

# Transport Security

`SocketSecuritySession` and `SocketSecurityDatagram` bind stream/datagram semantics to ESPressio Security without implementing cryptography themselves.

TLS and ESPressio Security continue to protect different boundaries: TLS protects a connection/session; ESPressio Security protects application transport payloads with protocol binding, sender/session identity and replay semantics.

# Observable lifecycle

Sockets exposes synchronous Observable lifecycle surfaces for socket infrastructure. Applications requiring asynchronous Event representations can opt into the Sockets-owned Event bridges.

# Published 0.7.3 cascade reference

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

- core Sockets remains portable;
- Event integration remains opt-in;
- Command integration remains opt-in;
- State integration remains opt-in;
- Timing synchronization remains opt-in;
- Security integration remains opt-in;
- the normal umbrella remains independent of those optional domains;
- Socket Command protocol-v1 wire format is unchanged;
- State retains ownership of State-domain protocol/revision/subscription semantics;
- target-specific network implementation is supplied below Sockets through ESPressio platform providers rather than being owned by the reusable Sockets layer.

The dependency direction remains:

```text
Sockets Event integration - - -> Event
Sockets Command integration - - -> Command
Sockets State integration - - -> State
Sockets Security integration - - -> Security
Sockets Timing integration - - -> Timing
```

There is no reverse dependency from State, Event, Command, Security or Timing back to Sockets.

# Testing

Host and ESP32 validation exercise core socket facilities and optional integrations. The active State-session host test validates synchronous subscription/snapshot flow, bidirectional publication, unsubscribe behavior and fragmented framing against the active ESPressio-State working branch.

# Changelog

See [CHANGELOG.md](CHANGELOG.md) for release history and notable changes.
