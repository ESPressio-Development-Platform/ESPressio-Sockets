# Changelog

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
