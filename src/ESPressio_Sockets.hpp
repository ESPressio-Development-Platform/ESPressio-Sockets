#pragma once

#include "ESPressio_SocketTypes.hpp"
#include "ESPressio_SocketEventFrame.hpp"

/*
 * Event transport adapters are deliberately NOT batch-included here.
 *
 * Include only the protocol adapter(s) required by the project:
 *
 *   ESPressio_UDPEventTransport.hpp
 *   ESPressio_TCPClientEventTransport.hpp
 *   ESPressio_TCPServerEventTransport.hpp
 *   ESPressio_TLSEventTransport.hpp
 *   ESPressio_WebSocketClientEventTransport.hpp
 *   ESPressio_WebSocketServerEventTransport.hpp
 *   ESPressio_MQTTEventTransport.hpp
 *
 * Timing synchronization is likewise opt-in through:
 *
 *   ESPressio_SocketClockSynchronization.hpp
 *
 * Command invocation is opt-in through:
 *
 *   ESPressio_SocketCommandTypes.hpp
 *   ESPressio_SocketCommandProtocol.hpp
 *   ESPressio_SocketCommandSession.hpp
 *   ESPressio_TCPCommandServer.hpp
 *
 * This keeps ESPressio Event/Serializable, ESPressio Timing, and ESPressio
 * Command dependencies opt-in at the consuming-code level.
 */
