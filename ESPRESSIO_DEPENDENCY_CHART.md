# ESPressio Dependency Chart — Sockets 0.4.0

Core ESPressio Sockets remains independent of mandatory ESPressio libraries. Integrations are selected explicitly by the consuming application.

```text
ESPressio Sockets 0.4.x
|
+-- optional --> ESPressio Event >= 5.7.1 < 6.0.0
|                  +-- socket Event Transport adapters
|
+-- optional --> ESPressio Command >= 0.2.0 < 1.0.0
|                  +-- SocketCommandSession / TCPCommandServer
|
+-- optional --> ESPressio Timing >= 2.2.2 < 3.0.0
|                  +-- socket clock synchronization
|
+-- optional --> ESPressio Security >= 0.1.0 < 1.0.0
                   +-- SocketSecuritySession
                   +-- SocketSecurityDatagram
```

External WebSocket/MQTT dependencies remain associated with the concrete adapters that use them.

## Security Placement

```text
Event / Command / application payload
                |
                v
        ESPressio Security
                |
         +------+------+
         |             |
         v             v
  stream session     datagram
         |             |
         v             v
 TCP/TLS/WS        UDP/message
```

Event and Command do not gain direct Security dependencies. Authentication/decryption occurs at the transport boundary before plaintext is handed to higher-level protocol processing.

## Optional Header Rule

The core umbrella:

```cpp
#include <ESPressio_Sockets.hpp>
```

does not include Event-, Command-, Timing-, or Security-dependent adapters.

Security is selected explicitly with:

```cpp
#include <ESPressio_SocketSecuritySession.hpp>
#include <ESPressio_SocketSecurityDatagram.hpp>
```

## PlatformIO

Core:

```ini
lib_deps =
    https://github.com/Flowduino/ESPressio-Sockets@^0.4.0
```

Security integration:

```ini
lib_deps =
    https://github.com/Flowduino/ESPressio-Sockets@^0.4.0
    https://github.com/Flowduino/ESPressio-Security@^0.1.0
```

Add Event, Command, or Timing only when the corresponding adapters are compiled.

## Version Policy

Optional ESPressio dependencies are bounded to their current supported major versions so future breaking major releases are not selected automatically.
