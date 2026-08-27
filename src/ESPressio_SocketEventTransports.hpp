#pragma once

// Concrete network Event transports are platform adapters. ESPressio-Sockets
// retains this historical umbrella as a source-compatibility forwarding point,
// but does not depend on a concrete platform package itself.
#if __has_include(<ESPressio_ESP32SocketTransports.hpp>)
#include <ESPressio_ESP32SocketTransports.hpp>
#else
#error "Concrete socket Event transports require a platform adapter package. On ESP32 include/install ESPressio-ESP32 and ESPressio_ESP32SocketTransports.hpp."
#endif
