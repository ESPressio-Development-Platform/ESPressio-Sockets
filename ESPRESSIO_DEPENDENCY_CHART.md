# ESPressio Dependency Chart — Sockets 0.7.0

![ESPressio Library Dependency Chart](ESPRESSIO_DEPENDENCY_CHART.svg)

## Sockets 0.7.0

```text
Sockets 0.7.0
    -> Observable >= 3.0.1 < 4.0.0
    - - -> Event >= 6.0.0 < 7.0.0
    - - -> Command >= 1.0.0 < 2.0.0
    - - -> Security >= 0.3.0 < 1.0.0
    - - -> Timing >= 2.2.4 < 3.0.0
```

Event covers concrete socket Event Transports plus Socket-owned lifecycle Event types and bridges. Command 1.x integration includes line mode and structured protocol-v1 invocation with `CommandValue` normalization at the wire boundary. All four domain integrations remain opt-in.

## Final coordinated ecosystem

```text
Observable 3.0.1
Serializable 0.10.2
Units 0.2.3
Timing 2.2.4
Threads 3.1.4
Command 1.0.0
Security 0.3.0
Event 6.0.0
Sockets 0.7.0
ESP-Now 0.7.0
Serial 0.7.0
```

## Circular-dependency status

```text
Sockets - - -> Event 6.0.0
Event   -> Sockets     NONE
```

Sockets owns `SocketWorkerEventBridge`, `SocketSecuritySessionEventBridge`, and their associated Event types. Event remains transport-neutral and has no Sockets dependency.
