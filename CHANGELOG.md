# Changelog

## 0.7.0 — 2026-08-22

### Changed
- Updated the opt-in ESPressio Command integration baseline to Command >= 1.0.0 < 2.0.0.
- Adapted structured Socket Command protocol v1 to Command 1.0.0 `CommandValue` containers.
- Native scalar `CommandValue` instances are normalized with `ToString()` at the existing protocol-v1 wire boundary and decoded as string-backed values, preserving interoperability with existing protocol-v1 peers.
- Null `CommandValue` is rejected because protocol v1 has no null representation.
- Added host and ESP32 validation against released ESPressio Command 1.0.0.
- Updated package/component metadata, README, Command integration documentation, CI, and dependency charts for Sockets 0.7.0.

### Compatibility
- Core Sockets remains independent of ESPressio Command.
- Socket Command line mode remains unchanged.
- Structured Socket Command wire protocol version remains 1; no wire-format migration is required for existing peers.
- Existing Event, Security, Timing and socket transport APIs remain unchanged.

### Tracking
- Implements #17.
- Cascades ESPressio Command 1.0.0.

## 0.6.0 — 2026-08-21

### Added
- Moved Socket worker and Socket security-session Event types, `SocketWorkerEventBridge`, and `SocketSecuritySessionEventBridge` ownership into ESPressio Sockets.
- Added Sockets-owned optional Event integration targeting ESPressio Event 6.0.0.

### Changed
- Preserved the existing Socket Event bridge/header names in their new owning package.
- Updated validated optional integration generation to Command 0.4.0, Security 0.3.0, Event 6.0.0, Timing 2.2.4, Units 0.2.3, and Observable 3.0.1.
- Core `ESPressio_Sockets.hpp` remains free of Event-, Command-, Security-, and Timing-specific integration headers.
- Updated package/component metadata, README, CI, and both dependency-chart forms for Sockets 0.6.0.

### Compatibility
- Existing socket transport, Event Transport, Command, Security, and Timing runtime semantics are unchanged.
- Applications using Socket lifecycle Event bridges must obtain them from ESPressio Sockets 0.6.0 rather than ESPressio Event 6.0.0.

### Tracking
- Implements #12.
- Coordinated with Flowduino/ESPressio-Event#34.

## 0.5.0 — 2026-08-20

### Added
- Added `ISocketWorkerObserver` and observable socket-worker lifecycle notifications for start, start failure, and stop transitions.
- Added `ISocketSecuritySessionObserver` and observable secure-session fault/reset notifications.
- Added ESPressio Observable >= 3.0.1 < 4.0.0 as the common lifecycle-observer dependency.
- Added optional ESPressio Event bridge support through ESPressio Event 5.8.0.

### Changed
- Updated the validated optional ESPressio Security baseline to Security >= 0.2.0 < 1.0.0.
- Security session send/unprotect/frame-limit failures now publish lifecycle observations while preserving existing result and callback behavior.
- Bumped package/component/public version metadata to 0.5.0.
- Event, Timing, Command and Security integrations remain opt-in.

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
