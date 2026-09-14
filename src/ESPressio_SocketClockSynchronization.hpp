#pragma once

#include "timing/ESPressio_SocketClockSynchronizationTypes.hpp"
#include "timing/ESPressio_SocketClockSynchronizationProtocol.hpp"

/*
 * R9-19 final socket/network Timing seam.
 *
 * Concrete TCP/UDP/WebSocket providers may carry this bounded request/response
 * wire, but must supply provider-proximate receive captures with truthful
 * ClockCaptureQuality and conservative uncertainty. Timing owns estimator,
 * reference qualification, discipline and adaptive evidence deadlines.
 *
 * The predecessor fixed-cadence TCP/UDP/SNTP wrappers are intentionally not
 * preserved: they either depended on removed Event framing or could not supply
 * honest K1/K2 four-capture evidence.
 */
