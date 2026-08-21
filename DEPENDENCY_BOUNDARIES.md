# ESPressio Sockets Dependency Boundaries

ESPressio Sockets owns Socket-domain lifecycle semantics, concrete socket Event transports, and the optional Event representations of Socket worker and security-session lifecycle.

The core Sockets umbrella must remain free of Event, Command, Timing, and Security integration headers. Those dependencies are introduced only when their specific adapters are selected.

For Event integration, Sockets is the downstream owner of its concrete Event transports, Socket lifecycle Event types, `SocketWorkerEventBridge`, and `SocketSecuritySessionEventBridge`. ESPressio Event must not depend back on Sockets merely to represent Socket-domain behavior.

The intended direction is therefore one-way:

```text
Sockets - - -> Event
Event       -> Sockets  NONE
```
