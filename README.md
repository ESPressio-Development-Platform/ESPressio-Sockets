# ESPressio Sockets

Socket-based transport implementations for the Flowduino ESPressio Development Platform.

ESPressio Sockets provides IP/socket-oriented communication adapters separately from hardware-radio-specific ESPressio libraries. Its initial release focuses on concrete `IEventTransport` implementations for ESPressio Event.

## Latest Stable Version

The current repository version is **0.3.0**.

## Compatibility

ESPressio Sockets `0.3.0` targets the **ESP32 family under Arduino-ESP32** and uses C++17.

The library uses Arduino-ESP32 native networking classes for UDP, TCP and TLS. WebSocket support is provided through the mature Links2004 `arduinoWebSockets` library.

## ESPressio Architecture

The intended responsibility boundary is:

```text
ESPressio Event
    |
    | IEventTransport
    v
ESPressio Sockets
    |
    +-- UDP
    +-- TCP client
    +-- TCP server
    +-- TLS client
    +-- WebSocket client
    +-- WebSocket server
    +-- MQTT / MQTT over TLS
           |
           v
      IP networking
```

Hardware-radio-specific protocols remain outside this library:

```text
ESP-NOW
    -> ESPressio ESP-Now

LoRa / packet radio / other hardware radios
    -> future ESPressio Radio
```

This keeps socket/network concerns separate from radio-hardware concerns.

## Dependencies

The core `ESPressio_Sockets.hpp` header contains only the common socket types and stream-framing utilities.

Event Transport adapters require:

```text
ESPressio Event >= 5.7.1 < 6.0.0
```

and therefore the Serializable support used by ESPressio Event Transport.

Command invocation is also opt-in and targets **ESPressio Command >= 0.2.0 < 1.0.0**. See [Command Integration](COMMAND_INTEGRATION.md) for the TCP server, line/structured protocols, session metadata, policy hooks, limits, and examples.

WebSocket adapters additionally use:

```text
Links2004 WebSockets >= 2.3.6
```

MQTT uses:

```text
PubSubClient >= 2.8
```

Both are declared as supported repository dependencies. `arduinoWebSockets` provides RFC6455 client/server functionality, while PubSubClient provides the MQTT publish/subscribe client used by the MQTT adapter.

## Namespace

The public API is contained within:

```cpp
ESPressio::Sockets
```

## Header Structure

Core umbrella:

```cpp
#include <ESPressio_Sockets.hpp>
```

All Event transports:

```cpp
#include <ESPressio_SocketEventTransports.hpp>
```

Or individual adapters:

```cpp
#include <ESPressio_UDPEventTransport.hpp>
#include <ESPressio_TCPClientEventTransport.hpp>
#include <ESPressio_TCPServerEventTransport.hpp>
#include <ESPressio_TLSEventTransport.hpp>
#include <ESPressio_WebSocketClientEventTransport.hpp>
#include <ESPressio_WebSocketServerEventTransport.hpp>
```

The normal `ESPressio_Sockets.hpp` umbrella intentionally does not pull Event Transport adapters into projects that only need the common socket layer.

# Event Transport Implementations

## UDPEventTransport

`UDPEventTransport` sends each Event Transport packet as one UDP datagram.

Supported addressing includes:

```text
unicast
IPv4 broadcast
IPv4 multicast destination
multicast receive binding
multiple outbound destinations
```

Example:

```cpp
Sockets::UDPEventTransport udp;

Sockets::UDPEventTransportConfig config;
config.LocalPort = 42000;

udp.Initialize(config);

udp.AddDestination(
    {
        IPAddress(192, 168, 1, 50),
        42000
    }
);

udp.AddBroadcastDestination(
    42000
);
```

For multicast reception:

```cpp
udp.InitializeMulticast(
    IPAddress(239, 42, 0, 1),
    42000
);

udp.AddMulticastDestination(
    IPAddress(239, 42, 0, 1),
    42000
);
```

UDP preserves packet boundaries naturally, so no additional ESPressio Sockets framing is added around the Event Transport packet.

The maximum accepted packet size defaults to:

```text
65536 bytes
```

through:

```cpp
ESPRESSIO_SOCKETS_MAX_EVENT_PACKET_SIZE
```

Actual usable UDP payload limits remain constrained by the IP stack and network path; applications should prefer smaller Event payloads when using UDP.

## TCPClientEventTransport

`TCPClientEventTransport` maintains a client connection to one TCP Event endpoint.

Configuration:

```cpp
Sockets::TCPClientEventTransportConfig config;

config.Host =
    "192.168.1.100";

config.Port =
    43000;

config.ReconnectIntervalMilliseconds =
    2000;
```

The adapter automatically retries connections and receives Event packets from the connected server.

TCP is a byte stream rather than a message protocol, so ESPressio Sockets adds a small versioned stream frame around each Event Transport packet.

## TCPServerEventTransport

`TCPServerEventTransport` listens for multiple clients and broadcasts each outbound Event Transport packet to every connected TCP client.

Configuration:

```cpp
Sockets::TCPServerEventTransportConfig config;

config.Port = 43000;
config.MaximumClients = 8;
```

Incoming framed Event packets from any connected client are passed into the local Event Transport Manager.

This makes a useful hub topology:

```text
             TCP Server
            /    |     \
           /     |      \
          v      v       v
      Client A Client B Client C
```

## TCP stream framing

TCP/TLS Event transports use:

```cpp
SocketEventFrameHeader
```

containing:

```text
magic
version
payload length
```

The frame exists only to recover Event Transport packet boundaries from a byte stream.

It does not replace the versioned `EventTransportEnvelope` owned by ESPressio Event.

The layering is:

```text
SocketEventFrameHeader
        |
        +-- EventTransportEnvelope
        +-- Serializable Event payload
```

## TLSEventTransport

`TLSEventTransport` provides the same Event semantics as the TCP client transport using Arduino-ESP32 `WiFiClientSecure`.

It supports:

```text
server CA verification
optional client certificate/private key
explicit insecure mode for development
automatic reconnection
```

Example:

```cpp
Sockets::TLSEventTransportConfig config;

config.Host =
    "event-server.example.com";

config.Port = 4433;

config.CACertificate =
    ROOT_CA;

tls.Initialize(config);
```

For development only:

```cpp
config.Insecure = true;
```

should be used only when certificate verification is intentionally disabled.

## WebSocketClientEventTransport

`WebSocketClientEventTransport` uses binary RFC6455 WebSocket messages for Event Transport packets.

Configuration:

```cpp
Sockets::WebSocketClientEventTransportConfig config;

config.Host =
    "192.168.1.100";

config.Port = 44000;
config.Path = "/";
config.Protocol = "espressio";
```

For secure WebSocket client operation:

```cpp
config.Secure = true;
config.Port = 443;
config.CACertificate = ROOT_CA;
```

The adapter supports:

```text
ws
wss
automatic reconnect
heartbeat/ping-pong
binary Event packets
```

## WebSocketServerEventTransport

`WebSocketServerEventTransport` exposes an ESP32 WebSocket server and broadcasts each outbound Event Transport packet as a binary WebSocket frame to every connected WebSocket client.

Configuration:

```cpp
Sockets::WebSocketServerEventTransportConfig config;

config.Port = 44000;
config.Protocol = "espressio";

webSocketServer.Initialize(
    config
);
```

Incoming binary WebSocket messages from any connected client become inbound Event Transport packets.

The initial server adapter provides plain `ws` operation. Secure `wss` client operation is supported, while TLS termination for a WebSocket server can be placed in front of the ESP32 or added in a later transport implementation.

## MQTTEventTransport

`MQTTEventTransport` maps Event Transport packets onto MQTT binary payloads.

It supports:

```text
MQTT over TCP
MQTT over TLS
username/password authentication
separate inbound/outbound topics
automatic reconnect
configurable MQTT buffer size
keep-alive/socket timeout configuration
```

A typical two-device topology uses complementary topics:

```text
Device A:
    publish   espressio/a-to-b
    subscribe espressio/b-to-a

Device B:
    publish   espressio/b-to-a
    subscribe espressio/a-to-b
```

Configuration:

```cpp
Sockets::MQTTEventTransportConfig config;

config.Host = "192.168.1.10";
config.Port = 1883;
config.ClientID = "espressio-device-a";

config.OutboundTopic =
    "espressio/a-to-b";

config.InboundTopic =
    "espressio/b-to-a";

config.BufferSize = 4096;
```

For MQTT over TLS:

```cpp
config.Secure = true;
config.Port = 8883;
config.CACertificate = ROOT_CA;
```

MQTT packet buffer size must be large enough for the complete ESPressio Event Transport packet plus MQTT protocol overhead.

# Event Transport Registration

Every socket adapter implements:

```cpp
ESPressio::Event::IEventTransport
```

and therefore registers with Event 5.4 in the same way as ESP-NOW or any future transport:

```cpp
auto& manager =
    Event::EventTransportManager::
        GetInstance();

manager.RegisterTransport(
    &udpTransport
);
```

A Serializable Event can then be routed specifically over that socket:

```cpp
manager.RegisterBidirectionalEvent<
    MySerializableEvent
>(
    &udpTransport
);
```

Or multiple types:

```cpp
manager.RegisterOutboundEvents<
    TelemetryEvent,
    DiagnosticsEvent,
    StatusEvent
>(
    &udpTransport
);
```

Different transport policy can be applied simultaneously:

```text
TelemetryEvent:
    UDP        OUT
    TCP        OUT
    WebSocket  NONE

CommandEvent:
    UDP        NONE
    TCP        IN
    WebSocket  IN/OUT
```

That policy remains entirely inside ESPressio Event.

# Worker Tasks

Each adapter that requires continuous inbound processing owns a small FreeRTOS worker task.

Common task parameters are represented by:

```cpp
SocketWorkerConfig
```

with:

```text
StackSize
Priority
Core
IdleDelayMilliseconds
```

Worker shutdown waits for the task to exit before destroying its underlying socket resources.

# Delivery Semantics

`IEventTransport::Send()` reports that the concrete socket implementation accepted/wrote the Event Transport packet.

The initial release does not add a separate application-level delivery acknowledgement.

Transport characteristics therefore remain protocol-specific:

```text
UDP:
    datagram submission only; no delivery guarantee

TCP:
    ordered reliable byte-stream delivery while connection survives

TLS:
    TCP reliability plus encrypted/authenticated transport

WebSocket:
    binary message transport over TCP/TLS
```

ESPressio Event itself remains unaware of those protocol details.

# Wi-Fi / Network Ownership

ESPressio Sockets does not automatically configure Wi-Fi credentials.

The consuming application owns network establishment:

```cpp
WiFi.begin(
    ssid,
    password
);
```

before initializing transports that require an active interface.

This also leaves room for future Ethernet-backed socket use without coupling socket adapters directly to Wi-Fi setup.

# Examples

The initial release contains:

```text
examples/
├── UDPEventTransport/
│   └── UDPEventTransport.ino
│
├── TCPClientEventTransport/
│   └── TCPClientEventTransport.ino
│
├── TCPServerEventTransport/
│   └── TCPServerEventTransport.ino
│
├── TLSEventTransport/
│   └── TLSEventTransport.ino
│
├── WebSocketClientEventTransport/
│   └── WebSocketClientEventTransport.ino
│
├── WebSocketServerEventTransport/
│   └── WebSocketServerEventTransport.ino
│
└── MQTTEventTransport/
    └── MQTTEventTransport.ino
```

The TCP client/server and WebSocket client/server examples are complementary starting points for two-device testing.

The UDP example demonstrates broadcast-capable Event Transport.

# PlatformIO

Typical configuration:

```ini
[env:esp32]
platform = espressif32
framework = arduino
board = esp32dev

build_flags =
    -std=gnu++17

lib_deps =
    flowduino/ESPressio-Sockets@^0.3.0
    flowduino/ESPressio-Event@^5.7.1
    links2004/WebSockets@^2.3.6
    knolleary/PubSubClient@^2.8
```

# Design Direction

ESPressio Sockets is intentionally concerned with transports that conceptually belong to an IP/socket/network stack.

Good candidates for future expansion include:

```text
additional UDP multicast/group helpers
IPv6 socket transports
Unix/host socket adapters where applicable
HTTP streaming transports
SSE where bidirectional semantics can be appropriately paired
MQTT Event gateways
QUIC when supported appropriately on ESP32
TLS server support
WebSocket Secure server support
```

Hardware radio protocols should not be added here.

The planned separation is:

```text
ESPressio Sockets
    -> IP/socket/network protocols

ESPressio ESP-Now
    -> ESP-NOW

ESPressio Radio
    -> LoRa and other hardware-radio transports
```

This makes the transport layer composable without turning one library into a collection of unrelated communication hardware and protocols.

# Summary

ESPressio Sockets `0.2.0` provides the socket/network transport layer of the ESPressio ecosystem.

The initial release provides:

- UDP Event Transport;
- broadcast and multicast UDP support;
- TCP client Event Transport;
- multi-client TCP server Event Transport;
- TLS client Event Transport;
- WebSocket client Event Transport;
- Secure WebSocket (`wss`) client support;
- multi-client WebSocket server Event Transport;
- MQTT and MQTT-over-TLS Event Transport;
- C++17 implementation;
- common stream framing;
- configurable worker tasks;
- complete example projects;
- compatibility with ESPressio Event 5.4 per-transport routing.

The architectural boundary is deliberate:

**ESPressio Event decides which Events travel. ESPressio Sockets moves them using socket/network protocols. Hardware-radio transports remain in their own ESPressio libraries.**

# System Clock Synchronization — 0.2.0

ESPressio Sockets `0.2.0` adds opt-in network implementations for the transport-independent System Clock synchronization API in ESPressio Timing `2.2.0`.

The synchronization layer is deliberately separate from Event Transport and is **not** included by the normal:

```cpp
#include <ESPressio_Sockets.hpp>
```

Applications that need socket-based System Clock synchronization include:

```cpp
#include <ESPressio_SocketClockSynchronization.hpp>
```

and provide:

```text
ESPressio Timing >= 2.2.2 < 3.0.0
```

The ownership boundary remains:

```text
ESPressio Timing
    |
    +-- SystemClock
    +-- ClockSynchronizationSample
    +-- offset/delay calculation
    +-- clock discipline
    +-- step/slew policy
    +-- synchronization state
    +-- Observer notifications
           |
           | IClockSynchronizationTarget
           v
ESPressio Sockets
    |
    +-- UDP exchange
    +-- TCP exchange
    +-- WebSocket exchange
    +-- SNTP external reference
```

Socket transports do not implement a second clock discipline. They only acquire synchronization observations and submit them into ESPressio Timing.

## Common four-timestamp protocol

UDP, TCP and WebSocket request/response synchronization use the same four timestamps:

```text
Client                             Reference

T1 request transmit  ------------------>
                                    T2 request receive
                                    T3 response transmit
T4 response receive  <------------------
```

The completed exchange is submitted as:

```cpp
Timing::ClockSynchronizationSample<
    Timing::ClockTick
>
```

so Timing owns the normal calculations:

```text
round-trip delay = (T4 - T1) - (T3 - T2)
offset           = ((T2 - T1) + (T3 - T4)) / 2
```

This also means all existing Timing Observer callbacks and the optional `SystemClockEventBridge` continue to work unchanged.

## UDPClockSynchronizer

`UDPClockSynchronizer` is the preferred socket transport for precision synchronization because it avoids TCP retransmission and stream-buffering behavior.

A reference device can use:

```cpp
Sockets::UDPClockSynchronizationConfig config;
config.Mode =
    Sockets::SocketClockSynchronizationMode::Reference;
config.LocalPort = 45100;

synchronizer.Initialize(config);
```

A client uses:

```cpp
Sockets::UDPClockSynchronizationConfig config;
config.Mode =
    Sockets::SocketClockSynchronizationMode::Client;
config.LocalPort = 45101;
config.ReferenceAddress =
    IPAddress(192, 168, 1, 50);
config.ReferencePort = 45100;
config.SynchronizationIntervalMilliseconds = 5000;
```

The client periodically performs the full request/response exchange and submits the resulting four timestamps into ESPressio Timing.

### UDP authoritative broadcast

A reference can also periodically broadcast its current System Clock:

```cpp
config.Mode =
    Sockets::SocketClockSynchronizationMode::Reference;
config.EnableAuthoritativeBroadcast = true;
config.BroadcastIntervalMilliseconds = 5000;
```

Clients listening on the same UDP port can consume these one-way synchronization observations.

One-way broadcast deliberately cannot compensate for network latency, so it is intended for lower-overhead group synchronization where the stronger request/response measurement is unnecessary.

### UDP multicast

The same authoritative mode can use an IPv4 multicast group:

```cpp
config.EnableAuthoritativeMulticast = true;
config.MulticastGroup =
    IPAddress(239, 45, 10, 1);
config.MulticastPort = 45100;
```

This is useful for synchronizing a defined group of devices without local-network-wide broadcast traffic.

## TCP Clock Synchronization

Version 0.2.0 adds:

```cpp
TCPClockSynchronizationClient
TCPClockSynchronizationServer
```

The server can service multiple clients using the same versioned ESPressio socket frame already used by TCP Event Transport.

Client configuration:

```cpp
Sockets::TCPClockSynchronizationClientConfig config;
config.Host = "192.168.1.50";
config.Port = 45110;
config.SynchronizationIntervalMilliseconds = 5000;
```

TCP remains a valid convenience transport where a connection is already useful, although UDP is generally preferable when minimizing network/scheduler jitter is the priority.

## WebSocket Clock Synchronization

Version 0.2.0 also adds:

```cpp
WebSocketClockSynchronizationClient
WebSocketClockSynchronizationServer
```

The synchronization messages are transferred as binary WebSocket messages.

The client supports both:

```text
ws://
wss://
```

using the same Links2004 WebSockets dependency already used by ESPressio Sockets Event Transport.

This is particularly useful when the System Clock authority is exposed through a WebSocket-capable network endpoint rather than raw UDP/TCP.

## SNTPClockSyncProvider

`SNTPClockSyncProvider` uses the ESP-IDF/lwIP SNTP implementation as an external absolute-time source.

Example:

```cpp
Sockets::SNTPClockSyncProvider provider;

Sockets::SNTPClockSyncProviderConfig config;
config.Server = "pool.ntp.org";
config.UpdateIntervalMilliseconds = 3600000;

provider.Initialize(config);
```

When ESP-IDF reports a successful SNTP synchronization, the received Unix reference time is submitted into ESPressio Timing rather than replacing the ESPressio clock-discipline architecture.

The provider therefore establishes the ESPressio System Clock in the Unix epoch domain while preserving:

```text
Timing step/slew policy
Timing synchronization state
Timing accepted/rejected sample accounting
Timing Observer callbacks
SystemClockEventBridge integration
```

The SNTP callback does not expose the underlying NTP four packet timestamps, so this provider represents the externally synchronized SNTP time as a zero-duration reference observation. Use UDP request/response between ESPressio devices when the ESPressio four-timestamp round-trip measurement itself is required.

Only one active `SNTPClockSyncProvider` is supported because the underlying ESP-IDF SNTP synchronization callback is process-global.

## Timing dependency remains opt-in

The Clock Synchronization headers are not included by `ESPressio_Sockets.hpp`.

Therefore a project using only socket primitives or Event Transport does not need ESPressio Timing solely because the synchronization implementations exist in the repository.

Conversely, a project using:

```cpp
#include <ESPressio_SocketClockSynchronization.hpp>
```

must supply ESPressio Timing `>=2.2.2 <3.0.0`.

## Synchronization examples

Version 0.2.0 adds:

```text
examples/
├── UDPClockSynchronization/
│   └── UDPClockSynchronization.ino
├── UDPClockBroadcast/
│   └── UDPClockBroadcast.ino
├── TCPClockSynchronizationClient/
│   └── TCPClockSynchronizationClient.ino
├── TCPClockSynchronizationServer/
│   └── TCPClockSynchronizationServer.ino
├── WebSocketClockSynchronizationClient/
│   └── WebSocketClockSynchronizationClient.ino
├── WebSocketClockSynchronizationServer/
│   └── WebSocketClockSynchronizationServer.ino
└── SNTPClockSynchronization/
    └── SNTPClockSynchronization.ino
```

These are intentionally separate from the existing Event Transport examples.
