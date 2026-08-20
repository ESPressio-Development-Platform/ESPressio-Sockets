#pragma once

#ifndef ESPRESSIO_SOCKETS_VERSION_MAJOR
#define ESPRESSIO_SOCKETS_VERSION_MAJOR 0
#endif
#ifndef ESPRESSIO_SOCKETS_VERSION_MINOR
#define ESPRESSIO_SOCKETS_VERSION_MINOR 5
#endif
#ifndef ESPRESSIO_SOCKETS_VERSION_PATCH
#define ESPRESSIO_SOCKETS_VERSION_PATCH 0
#endif
#ifndef ESPRESSIO_SOCKETS_VERSION_STRING
#define ESPRESSIO_SOCKETS_VERSION_STRING "0.5.0"
#endif

#include "ESPressio_SocketTypes.hpp"
#include "ESPressio_SocketEventFrame.hpp"

/*
 * Dependency-bearing integrations are deliberately NOT batch-included here.
 * Include only the facilities required by the project.
 *
 * Observable lifecycle:
 *   ESPressio_SocketWorker.hpp
 *   ESPressio_ISocketWorkerObserver.hpp
 *   ESPressio_SocketSecuritySession.hpp
 *   ESPressio_ISocketSecuritySessionObserver.hpp
 *
 * Event transports:
 *   ESPressio_UDPEventTransport.hpp
 *   ESPressio_TCPClientEventTransport.hpp
 *   ESPressio_TCPServerEventTransport.hpp
 *   ESPressio_TLSEventTransport.hpp
 *   ESPressio_WebSocketClientEventTransport.hpp
 *   ESPressio_WebSocketServerEventTransport.hpp
 *   ESPressio_MQTTEventTransport.hpp
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
 * Security (validated against ESPressio Security >=0.2.0 <1.0.0):
 *   ESPressio_SocketSecuritySession.hpp
 *   ESPressio_SocketSecurityDatagram.hpp
 *
 * Event, Timing, Command, and Security integrations remain opt-in at the
 * consuming-code level. Observable is the common lifecycle-notification
 * dependency used by socket workers and secure sessions.
 */
