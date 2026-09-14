# ESPressio Dependency Chart — Primitive redesign

This document records the final Sockets dependency position for Structural Tranche 9. Release-version history is intentionally excluded.

## Core package dependency position

```text
ESPressio-Sockets
    -> ESPressio-System     primitives_redesign
    -> ESPressio-Observable primitives_redesign
```

Security and Timing are explicit opt-in source integrations. Event, Command and State are no longer Sockets transport dependencies because their family semantics compose through generic Adapter bindings above the neutral transport.

## Neutral transport composition

```text
Event / Command / State
        |
        v
family Adapter bindings
        |
        v
ESPressio-Adapters (A2)
        |
        v
ESPressio-Sockets::SocketAdapterTransport
```

Optional genuine domain integrations:

```text
Sockets - - -> Security  (SocketSecuritySession / SocketSecurityDatagram)
Sockets - - -> Timing    (bounded K1/K2 socket evidence protocol)
```

## Dependency-direction invariants

```text
Sockets core -> Event    NONE
Sockets core -> Command  NONE
Sockets core -> State    NONE

Event   -> Sockets       NONE required by family runtime
Command -> Sockets       NONE required by family runtime
State   -> Sockets       NONE required by family runtime
```

WebSocket ownership remains in ESPressio-Web. Web may consume reusable socket/session mechanics without transferring Web protocol ownership back to Sockets.
