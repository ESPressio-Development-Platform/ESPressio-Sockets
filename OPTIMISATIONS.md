# ESPressio Sockets Optimisations

Chronological record of resource, ownership and transport-efficiency changes made while preparing the current working branch for reintegration.

## 2026-08-27 — #30 Socket State transport

- Added an opt-in State integration that reuses ESPressio State's existing transport-neutral protocol rather than duplicating State semantics in Sockets.
- Added bounded TCP framing with explicit magic/version/length validation and fragmented-stream accumulation.
- Added one `SocketStateSession` per peer/connection, preserving State epoch/revision ordering, subscriptions, acknowledgements, resynchronisation and disconnect semantics.
- Incoming State values are decoded once and moved into `RemoteStateManager` rather than retained in transport-specific repositories.
- Authoritative initial/resynchronisation values are obtained from `StatePublisher::Snapshot`, so stale historical State is not queued by the socket layer.
- Added TCP client/server adapters while keeping State integration opt-in; the normal Sockets umbrella does not acquire a mandatory State dependency.
- Added RTTI-free host regression coverage and a practical TCP State server example.
- The integration test exposed State #12: subscription-registry observer callbacks previously ran under the registry mutex and could deadlock a synchronous transport. The fix belongs to State and is consumed here rather than adding a transport workaround.
