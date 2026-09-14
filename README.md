# ESPressio Sockets

`ESPressio-Sockets` provides bounded, family-neutral socket/session transport mechanics for the ESPressio Development Platform.

Sockets owns generic non-Web socket concerns: byte framing, stream/datagram session lifecycle, finite ingress/egress, backpressure, endpoint routing, optional ESPressio-Security session/datagram protection, and optional K1/K2 Timing exchange. Web protocols remain owned by `ESPressio-Web`.

Event, Command and State representation/admission/execution semantics are **not** implemented as parallel Sockets family runtimes. Those families compose through generic Adapter bindings above this transport layer.

## Final Tranche-9 ownership

```text
Event / Command / State
        |
        v
  family Adapter binding
        |
        v
ESPressio-Adapters (A2)
        |
        v
SocketAdapterTransport
        |
        v
 caller-owned socket writer / connection
```

`SocketAdapterTransport` carries opaque family representation bytes plus the neutral service/family/protocol metadata and exact destination-admission receipt required by A2. It does not interpret Event occurrences, Command execution, State convergence or family retry policy.

## Package dependencies

The core package dependency boundary is:

```text
ESPressio-System
ESPressio-Observable
```

During the Primitive redesign tranche, dependency URLs use their matching `primitives_redesign` branches.

Security and Timing are optional source integrations and are compiled only when their integration headers are selected. Event, Command and State are not Sockets transport dependencies.

## Public umbrella

```cpp
#include <ESPressio_Sockets.hpp>
```

The core umbrella exposes portable socket types, worker infrastructure and stream helpers. Family-specific Event/Command/State transport stacks are intentionally absent.

The neutral A2 lower transport is explicit:

```cpp
#include <ESPressio_SocketAdapterTransport.hpp>
```

Optional integration surfaces include:

```cpp
#include <ESPressio_SocketSecuritySession.hpp>
#include <ESPressio_SocketSecurityDatagram.hpp>
#include <ESPressio_SocketClockSynchronization.hpp>
```

## Neutral Adapter transport

`SocketAdapterTransport` is compile-time bounded:

```cpp
using Transport = ESPressio::Sockets::SocketAdapterTransport<
    AdapterRuntime,
    MaximumSessions,
    MaximumPendingOutbound,
    MaximumPendingInboundReceipts,
    MaximumFrameBytes
>;
```

All four capacities are part of the Type. The transport owns fixed session/correlation tables, one fixed TX framing workspace, and one fixed stream-assembly buffer per session. It does not fall back to an unbounded queue when those capacities are exhausted.

A session is bound before configuration freezes:

```cpp
Transport transport(adapterRuntime, policyResolver, serviceWake);

transport.BindSession(
    {routeToken},
    Sockets::SocketAdapterSessionMode::Datagram,
    writer,
    provenance,
    true
);

transport.Freeze();
transport.Start();
```

`writer` is a `SocketAdapterWriter` supplied by the concrete connection/session owner. The neutral transport never opens an operating-system socket on behalf of a Primitive family.

For stream connections, feed arbitrary byte chunks through `FeedStream(...)`; the transport keeps one bounded assembly buffer per frozen session. Datagram transports use `FeedDatagram(...)` and require one complete frame per datagram.

## Exact destination admission

A Primitive-family policy may require `DestinationPrimitiveAdmission`. Sockets carries an exact bounded admission receipt rather than treating successful socket write/delivery as family admission.

Only these destination dispositions establish admission evidence:

```text
Accepted
AlreadyAccepted
```

`TemporarilyUnavailable`, `ResourceUnavailable`, `Unsupported`, `Rejected` and `Malformed` remain exact non-establishing results.

A successful socket write therefore means only that the lower transport accepted the framed bytes. It does not by itself establish Event/Command/State admission.

## Lifecycle and stale completion safety

Every transport and bound session has a finite lifecycle generation.

- `Quiesce()` rejects new work and resolves/releases transport-owned volatile correlation state.
- `Restart(...)` advances the transport generation.
- session availability changes advance the session generation where required.
- late admission receipts or inbound completions from a previous transport/session generation are rejected as stale.
- correlation counters fail closed at exhaustion rather than wrapping into current work.
- temporary writer backpressure remains a bounded transport result; it is not converted into a hidden retry worker.

## Backpressure and concurrency

The transport deliberately uses finite contention points:

- one TX workspace protected by `atomic_flag`;
- at most one parser per session at a time;
- `TMaximumPendingOutbound` exact-M1 correlations;
- `TMaximumPendingInboundReceipts` destination-receipt correlations;
- one `TMaximumFrameBytes` stream buffer per session.

When the writer or a finite transport resource cannot accept work, the lower transport returns the corresponding bounded availability/resource disposition to A2.

## Socket worker

`SocketWorker` remains generic socket lifecycle/maintenance infrastructure. Its synchronous Observable observer surface reports worker lifecycle state without manufacturing application Event traffic. It is not an Event/Command/State executor.

## Security

`SocketSecuritySession` and `SocketSecurityDatagram` adapt stream/datagram bytes to `ESPressio-Security` without implementing cryptography themselves.

TLS and ESPressio-Security protect different boundaries: TLS protects a connection/session; ESPressio-Security protects application transport payloads with its own protocol binding, sender/session identity and replay semantics.

## K1/K2 Timing evidence

`SocketClockSynchronizationProtocol` implements one bounded network clock evidence exchange. It carries the required T1/T2/T3/T4 coordinates together with capture quality, conservative uncertainty and reference reliability.

Timing remains responsible for:

- deciding when evidence is due;
- validating evidence quality;
- affine estimation;
- qualification/holdover;
- clock discipline;
- source switching and uncertainty propagation.

Sockets does not reconstruct historical synchronized System time and does not run a fixed-cadence clock discipline loop.

## Removed predecessor family stacks

The following old Sockets-owned family runtimes are intentionally absent from the final Tranche-9 source tree:

```text
SocketEvent* transports / Event bridges
SocketCommandProtocol / SocketCommandSession / TCPCommandServer
SocketState* sessions / TCPStateClient / TCPStateServer
family-specific socket retry/execution stacks
```

Applications should use the generic Event/Command/State Adapter bindings and bind A2 to `SocketAdapterTransport` rather than recreating those removed paths.

## WebSocket ownership

WebSocket protocol/routes/clients/servers remain owned by `ESPressio-Web`. Web may consume reusable Sockets byte/session mechanics where appropriate, but Sockets does not reclaim Web protocol ownership.

## Testing

The redesign branch validates:

- neutral A2 socket transport and exact-M1 carriage;
- stream/datagram framing and bounded recovery;
- lifecycle generation and stale receipt/completion rejection;
- deterministic congestion/backpressure behavior;
- Security session/datagram integration;
- K1/K2 Timing protocol behavior;
- package and predecessor-eradication dependency guards.

All redesign dependency checkouts used by the Tranche-9 workflows target `primitives_redesign`.
