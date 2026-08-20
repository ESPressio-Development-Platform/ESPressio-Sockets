# Changelog

## 0.4.0 — 2026-08-20

### Added
- Added opt-in ESPressio Security integration targeting Security >= 0.1.0 < 1.0.0.
- Added `SocketSecuritySession` for TCP/TLS/WebSocket-style byte streams with explicit protected-frame length framing and arbitrary receive chunking.
- Added `SocketSecurityDatagram` for UDP/message-oriented carriers with one ESPressio Security envelope per datagram.
- Added authenticated receive callbacks and security-failure observation without exposing key material.
- Added support for Security sender IDs, authenticated session epochs, key IDs, AEAD algorithm abstraction and replay protection.
- Added secure TCP client example and host tests for fragmented/coalesced stream frames, frame limits, datagram protection, and replay rejection.

### Changed
- Bumped package/component/public version metadata to 0.4.0.
- Kept Security optional; the normal `ESPressio_Sockets.hpp` umbrella does not include Security-dependent headers.
- Existing Event, Command, Timing, TCP/UDP/TLS/WebSocket/MQTT functionality remains source-compatible.
- Documented ESPressio Security as independent from TLS and usable either alone or as defense-in-depth.

### Compatibility
- Existing 0.3.x applications continue to operate unchanged when Security integration is not selected.
- Security does not become a mandatory dependency of core Sockets.

## 0.3.0 — 2026-08-20

### Added
- Added opt-in ESPressio Command 0.2.x integration for remote Command invocation over sockets.
- Added host-testable `SocketCommandSession` with line-oriented and structured-binary request modes.
- Added `TCPCommandServer` with isolated per-client Command sessions.
- Added request/connection metadata, policy hooks, result observers, correlation IDs, bounded request handling, and structured request/response framing.
- Added a TCP Command server example and comprehensive host tests for Command framing, dispatch, validation, session isolation, policy and error paths.
- Added a permanent GitHub Actions host-test workflow pinned to released ESPressio dependencies.

### Changed
- Updated package/component version metadata to 0.3.0.
- Updated README and ESPressio dependency documentation for optional Command integration.
- Expanded host regression testing to retain coverage of the existing socket clock-synchronization protocol.

### Compatibility
- Core ESPressio Sockets remains independent of ESPressio Command.
- Existing Event Transport and Timing synchronization APIs remain source-compatible.
- ESPressio Command is required only when Command integration headers are selected.

## 0.2.3 — 2026-08-20

### Changed
- Updated the validated optional ESPressio Event Transport baseline from Event 5.6.2 to Event 5.7.1 within the 5.x line.
- Updated the validated optional ESPressio Timing synchronization baseline from Timing 2.2.1 to Timing 2.2.2 within the 2.x line.
- Updated package metadata for Sockets 0.2.3.
- Core Sockets remains independent of mandatory ESPressio dependencies; Event and Timing integrations remain opt-in.
- No UDP, TCP, TLS, WebSocket, MQTT, framing, or synchronization runtime semantics changed.

## 0.2.2 — 2026-08-19

### Changed
- Updated active ESPressio dependency baselines to the latest released versions available on 2026-08-19.
- Bounded dependency compatibility to the current major version so future breaking major releases are not selected automatically.
- Updated optional ESPressio Event integrations to require Event 5.6.2 or newer within the 5.x line.
- Updated optional ESPressio Timing integrations to require Timing 2.2.1 or newer within the 2.x line.
- Corrected compile-time patch-version macros to match the package version.

All notable changes to this project are documented in this file.

The structure follows the principles of [Keep a
Changelog](https://keepachangelog.com/en/1.1.0/) and [Semantic
Versioning](https://semver.org/).

> **Historical note:** This changelog was reconstructed retrospectively
> from published GitHub Releases, tags, release notes, repository
> history, and the documented public API. Where an historical release
> had little or no release-note detail, the entry is intentionally terse
> rather than inferring unsupported intent.

## [0.2.1] - 2026-08-19

### Changed

- Updated all optional socket Event Transport adapters to require ESPressio Event 5.5.0 or newer.
- Updated Event Transport documentation and PlatformIO dependency examples for ESPressio Event 5.5.0.
- Bumped ESPressio Sockets package/version metadata to 0.2.1.

### Compatibility

- No UDP, TCP, TLS, WebSocket, MQTT, or System Clock synchronization interfaces are changed by this patch release.
- Timing integration remains opt-in and unchanged.
- Core Sockets usage remains independent of ESPressio Event.

## \[0.2.0\] - 2026-08-19

### Added

-   Added opt-in ESPressio Timing System Clock synchronization.
-   Added UDP four-timestamp request/response synchronization.
-   Added UDP authoritative broadcast and multicast synchronization.
-   Added TCP client/server clock synchronization.
-   Added WebSocket client/server clock synchronization.
-   Added secure WebSocket (`wss`) synchronization client support.
-   Added SNTP/NTP external clock-reference integration.
-   Added a shared versioned socket clock-synchronization protocol.
-   Added support for custom `IClockSynchronizationTarget`
    implementations.
-   Added seven clock-synchronization examples.

### Changed

-   Kept synchronization policy and clock discipline inside ESPressio
    Timing.
-   Preserved Event Transport functionality from 0.1.0 and kept Timing
    integration opt-in.

## \[0.1.0\] - 2026-08-19

### Added

-   Initial ESPressio Sockets release.
-   Added a dedicated IP/socket networking layer for ESPressio Event
    Transport.
-   Added UDP Event Transport, including unicast, broadcast, and
    multicast operation.
-   Added TCP client and multi-client TCP server Event Transports.
-   Added TLS Event Transport.
-   Added WebSocket client/server Event Transports, including secure
    client support.
-   Added MQTT Event Transport.
-   Added stream framing for stream-oriented transports.
-   Added examples for the supported Event Transport mechanisms.

### Changed

-   Kept Event routing/type policy in ESPressio Event rather than
    embedding it in socket implementations.
-   Kept network-interface ownership outside Sockets so applications can
    establish Wi-Fi/Ethernet independently.
