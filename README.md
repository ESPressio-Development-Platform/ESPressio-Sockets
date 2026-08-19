# ESPressio Sockets

Socket-based Event Transport and System Clock synchronization
implementations for the ESPressio ecosystem.

## Latest Stable Version

**0.2.0**

## ESPressio Development Platform

ESPressio is a collection of discrete, composable component libraries
built around a common development ethos:

-   **Light-weight** --- minimise memory consumption and runtime
    overhead without sacrificing correctness.
-   **Ease of use** --- provide strongly typed, developer-friendly
    abstractions over lower-level facilities.
-   **Object-oriented** --- a type for everything, and everything in a
    type.
-   **SOLID** --- favour focused responsibilities, extensibility,
    substitutable abstractions, narrow interfaces, and dependency
    inversion wherever practical on embedded C++ platforms.

## License

Licensed under the **Apache License 2.0**. See [LICENSE](LICENSE).

## ESPressio Library Dependencies

ESPressio is designed as a modular ecosystem of independently useful
libraries, with required dependencies kept explicit and optional
integrations introduced only when the corresponding functionality is
selected.

For a complete overview of required dependencies, opt-in dependencies,
and the overall hierarchy, see:

**[ESPressio Library Dependency Chart](ESPRESSIO_DEPENDENCY_CHART.md)**

-   **Solid relationships** represent required ESPressio dependencies.
-   **Dashed relationships** represent opt-in dependencies introduced
    only by the corresponding feature, integration, type, or header.

### Required ESPressio dependencies

None for the core Sockets layer.

### Optional ESPressio dependencies

-   **ESPressio Event** for socket Event Transport adapters.
-   **ESPressio Timing** for socket/SNTP clock synchronization.

## Architectural boundary

Sockets owns IP/socket-oriented mechanisms:

``` text
UDP
TCP
TLS
WebSocket
MQTT
```

Hardware-specific radio transports remain in ESPressio ESP-Now and the
future ESPressio Radio library.

## Network ownership

Sockets does not own Wi-Fi configuration. The application establishes an
IP-capable interface first, keeping the architecture suitable for Wi-Fi,
Ethernet, or another IP interface.

## Event Transports

Opt-in Event adapters implement `IEventTransport` for:

-   UDP unicast/broadcast/multicast;
-   TCP client;
-   multi-client TCP server;
-   TLS client;
-   WebSocket client/server;
-   MQTT.

Routing policy remains in Event's `EventTransportManager`.

TCP/TLS use a small versioned stream frame because TCP does not preserve
message boundaries. Datagram/message-oriented protocols can carry the
Event Transport packet directly.

## Multiple transports

Sockets is designed for Event 5.4's multi-transport routing model:

``` text
EventTransportManager
    +--> UDP
    +--> TCP
    +--> WebSocket
    +--> MQTT
```

An Event type may have different direction policy on each transport.

## Timing integration

Version 0.2.0 adds opt-in System Clock synchronization over:

-   UDP four-timestamp request/response;
-   UDP authoritative broadcast/multicast;
-   TCP;
-   WebSocket;
-   SNTP external reference acquisition.

``` cpp
#include <ESPressio_SocketClockSynchronization.hpp>
```

Timing remains responsible for clock discipline and Observer
notifications.

## SNTP

`SNTPClockSyncProvider` uses the ESP32 SNTP stack to obtain an external
absolute-time reference and feeds that reference into ESPressio Timing.
It does not replace Timing's System Clock.

## Optional include model

``` cpp
#include <ESPressio_Sockets.hpp>
```

does not itself impose Event or Timing.

Event transports:

``` cpp
#include <ESPressio_SocketEventTransports.hpp>
```

Timing synchronization:

``` cpp
#include <ESPressio_SocketClockSynchronization.hpp>
```

## Third-party dependencies

WebSocket and MQTT adapters use their declared third-party protocol
libraries. Their maintenance status and compatible versions should be
revalidated when publishing a new Sockets release rather than treated as
permanently fixed.

## Design goals

-   Separate socket/IP mechanisms from radio-specific transports.
-   Keep Event routing policy in Event.
-   Keep clock discipline in Timing.
-   Keep Event and Timing integrations opt-in.
-   Support multiple simultaneous transports.
-   Avoid coupling Sockets specifically to Wi-Fi.
