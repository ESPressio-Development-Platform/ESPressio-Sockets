# ESPressio Sockets

Socket-based ESPressio transports, Command adapters, transport-security sessions, System Clock synchronization providers, and observable transport lifecycle surfaces for the Flowduino ESPressio Development Platform.

## Current Development Version

This branch targets **ESPressio Sockets 0.5.0**.

0.5.0 adds native Observable lifecycle coverage for socket workers and secure stream sessions, and refreshes the optional Security baseline to ESPressio Security 0.2.x.

See [CHANGELOG.md](CHANGELOG.md) for release history.

## Dependency model

ESPressio Sockets keeps feature dependencies as narrow as possible.

### Core lifecycle dependency

Sockets 0.5.0 requires:

- **ESPressio Observable >= 3.0.1 and < 4.0.0** for native worker/session lifecycle observation.

The Arduino networking integrations continue to use the existing WebSockets and PubSubClient dependencies where those adapters are selected.

### Optional ESPressio integrations

- **ESPressio Timing >= 2.2.2 and < 3.0.0** — clock synchronization providers.
- **ESPressio Command >= 0.3.0 and < 1.0.0** — socket Command invocation/session facilities.
- **ESPressio Security >= 0.2.0 and < 1.0.0** — protected stream/datagram sessions.
- **ESPressio Event >= 5.8.0 and < 6.0.0** — Event transports and optional observer-to-Event bridges.

Security, Command, Timing and Event are deliberately not batch-included by `ESPressio_Sockets.hpp`; consuming code includes only the integration headers it needs.

```text
Observable -----------------------> Sockets lifecycle
Timing -------- optional ---------> Sockets synchronization
Command ------- optional ---------> Sockets command sessions
Security ------ optional ---------> Sockets secure sessions
Event --------- optional ---------> socket Event transports / Event bridges
```

See [ESPRESSIO_DEPENDENCY_CHART.md](ESPRESSIO_DEPENDENCY_CHART.md) for the broader ecosystem view.

## PlatformIO

For core Sockets 0.5.0:

```ini
lib_deps =
    flowduino/ESPressio-Sockets@^0.5.0
    flowduino/ESPressio-Observable@^3.0.1
```

Add only the optional ESPressio libraries needed by the selected integration. For example, a secure stream session additionally consumes Security 0.2.x.

Before tags are published, coordinated feature-branch testing can use the Git sources explicitly.

## Core headers

`ESPressio_Sockets.hpp` exposes lightweight core socket types and documents the available opt-in integration headers.

Important integration headers include:

- `ESPressio_SocketWorker.hpp`
- `ESPressio_ISocketWorkerObserver.hpp`
- `ESPressio_SocketSecuritySession.hpp`
- `ESPressio_ISocketSecuritySessionObserver.hpp`
- `ESPressio_SocketSecurityDatagram.hpp`
- `ESPressio_SocketCommandSession.hpp`
- `ESPressio_TCPCommandServer.hpp`
- `ESPressio_SocketClockSynchronization.hpp`
- the UDP/TCP/TLS/WebSocket/MQTT Event Transport headers.

## Socket worker lifecycle observation

`SocketWorker` owns a FreeRTOS worker task and now exposes lifecycle notifications through `ISocketWorkerObserver`:

```cpp
class WorkerObserver final :
    public ESPressio::Sockets::ISocketWorkerObserver {
public:
    void OnSocketWorkerStarted(const char* name) override {
        // Worker task successfully created.
    }

    void OnSocketWorkerStartFailed(const char* name) override {
        // Worker task could not be created.
    }

    void OnSocketWorkerStopped() override {
        // Worker transitioned out of the running state.
    }
};
```

The observer surface is passive. It does not replace the worker implementation's own iteration logic or transport callbacks.

## Secure stream sessions

`SocketSecuritySession` applies ESPressio Security to stream-oriented carriers such as TCP/TLS/WebSocket byte streams while retaining explicit frame length boundaries.

The existing primary callbacks remain unchanged:

- `WriteCallback` — strategy/dependency used to write bytes to the carrier.
- `ReceiveCallback` — primary authenticated payload delivery path.
- `FailureCallback` — existing direct failure notification.

0.5.0 adds `ISocketSecuritySessionObserver` for passive lifecycle/diagnostic consumers:

```cpp
class SessionObserver final :
    public ESPressio::Sockets::ISocketSecuritySessionObserver {
public:
    void OnSocketSecuritySessionFaulted(
        const ESPressio::Security::SecurityResult& result
    ) override {
        // Metrics / diagnostics / audit reporting.
    }

    void OnSocketSecuritySessionReset() override {
        // Session framing state was explicitly reset.
    }
};
```

Security protection/authentication failures and invalid protected-frame limits are observable without changing the existing return-value and callback semantics.

## Datagram security

`SocketSecurityDatagram` remains the message-oriented companion for UDP-style carriers, with one Security envelope per datagram. Cryptographic session establishment, replay protection and authentication failures originate in ESPressio Security; applications that need to observe those underlying state transitions can subscribe to the `TransportSecurity` instance directly.

## Command integration

Socket Command facilities remain opt-in and consume ESPressio Command. The 0.5.0 dependency generation expects Command 0.3.x, which includes Observable command-registry lifecycle support. Sockets does not duplicate those registry notifications.

## Event integration

Socket Event transports remain opt-in. Additionally, ESPressio Event 5.8.0 supplies observer-to-Event bridges for the new Sockets lifecycle surfaces:

```cpp
#include <ESPressio_SocketWorkerEventBridge.hpp>
#include <ESPressio_SocketSecuritySessionEventBridge.hpp>
```

`SocketWorkerEventBridge` binds to a specific `SocketWorker`; `SocketSecuritySessionEventBridge` binds to a specific secure session. This keeps ownership explicit and prevents ESPressio Event from becoming a dependency of Sockets itself.

## Timing integration

Existing UDP/TCP/WebSocket/SNTP clock synchronization remains delegated to ESPressio Timing. Sockets provides transport mechanics; Timing owns synchronization policy and clock discipline.

## Event transports

Existing UDP, TCP, TLS, WebSocket and MQTT Event transports continue to use ESPressio Event's transport abstractions and routing policy. 0.5.0 does not change their wire protocol.

## Testing

The host suite validates core inclusion, Command integration, Security integration, clock synchronization and the new secure-session observer contract. CI now validates against the coordinated Command 0.3 / Security 0.2 / Observable 3.0.1 dependency generation.

## Compatibility

0.5.0 is intended as a backward-compatible extension of 0.4.x. Existing receive/write callbacks and transport integration APIs remain supported. Applications using the newly observable worker/session classes now need Observable 3.x available to the build.

## License

Apache License 2.0. See [LICENSE](LICENSE).
