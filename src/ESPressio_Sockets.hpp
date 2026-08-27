#pragma once

#ifndef ESPRESSIO_SOCKETS_VERSION_MAJOR
#define ESPRESSIO_SOCKETS_VERSION_MAJOR 0
#endif
#ifndef ESPRESSIO_SOCKETS_VERSION_MINOR
#define ESPRESSIO_SOCKETS_VERSION_MINOR 7
#endif
#ifndef ESPRESSIO_SOCKETS_VERSION_PATCH
#define ESPRESSIO_SOCKETS_VERSION_PATCH 3
#endif
#ifndef ESPRESSIO_SOCKETS_VERSION_STRING
#define ESPRESSIO_SOCKETS_VERSION_STRING "0.7.3"
#endif

#include "ESPressio_SocketTypes.hpp"
#include "ESPressio_SocketEventFrame.hpp"

/*
 * Dependency-bearing integrations are deliberately NOT batch-included here.
 * Include only the facilities required by the project.
 *
 * Portable lifecycle/session infrastructure:
 *   ESPressio_SocketWorker.hpp
 *   ESPressio_ISocketWorkerObserver.hpp
 *   ESPressio_SocketSecuritySession.hpp
 *   ESPressio_ISocketSecuritySessionObserver.hpp
 *
 * Concrete Event transports are platform adapters. On ESP32 use:
 *   <ESPressio_ESP32SocketTransports.hpp>
 *
 * The historical ESPressio_SocketEventTransports.hpp umbrella is retained as
 * an optional compatibility forwarder when a platform adapter package is
 * available; ESPressio-Sockets itself does not depend on ESPressio-ESP32.
 *
 * Timing:
 *   ESPressio_SocketClockSynchronization.hpp
 *
 * Command:
 *   ESPressio_SocketCommandTypes.hpp
 *   ESPressio_SocketCommandProtocol.hpp
 *   ESPressio_SocketCommandSession.hpp
 *   ESPressio_TCPCommandServer.hpp
 *
 * Security:
 *   ESPressio_SocketSecuritySession.hpp
 *   ESPressio_SocketSecurityDatagram.hpp
 *
 * Event, Timing, Command, and Security integrations remain opt-in at the
 * consuming-code level. Observable is the common lifecycle-notification
 * dependency used by socket workers and secure sessions.
 */
