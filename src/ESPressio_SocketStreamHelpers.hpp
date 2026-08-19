#pragma once

#include <cstddef>
#include <cstdint>

#include <Client.h>

namespace ESPressio::Sockets {

inline bool WriteAll(
    Client& client,
    const uint8_t* data,
    std::size_t size
) {
    std::size_t written = 0;

    while (written < size) {
        const std::size_t current =
            client.write(
                data + written,
                size - written
            );

        if (current == 0) {
            return false;
        }

        written += current;
    }

    return true;
}

}
