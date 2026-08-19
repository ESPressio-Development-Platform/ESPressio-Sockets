# Changelog

All notable changes to this project are documented in this file.

The structure follows the principles of [Keep a
Changelog](https://keepachangelog.com/en/1.1.0/) and [Semantic
Versioning](https://semver.org/).

> **Historical note:** This changelog was reconstructed retrospectively
> from published GitHub Releases, tags, release notes, repository
> history, and the documented public API. Where an historical release
> had little or no release-note detail, the entry is intentionally terse
> rather than inferring unsupported intent.

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
