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

/*
 * Core ESPressio-Sockets is family-neutral.
 *
 * Neutral A2 lower transport:
 *   ESPressio_SocketAdapterTransport.hpp
 *
 * Portable lifecycle/session infrastructure:
 *   ESPressio_SocketWorker.hpp
 *   ESPressio_ISocketWorkerObserver.hpp
 *
 * Security-session mechanics:
 *   ESPressio_SocketSecuritySession.hpp
 *   ESPressio_SocketSecurityDatagram.hpp
 *   ESPressio_ISocketSecuritySessionObserver.hpp
 *
 * Timing evidence (pending ordered R9-19 migration):
 *   ESPressio_SocketClockSynchronization.hpp
 *
 * Event, Command and State representation/admission/execution semantics are
 * not owned by ESPressio-Sockets. They compose through the generic Adapter
 * runtime and family-owned bindings.
 */
