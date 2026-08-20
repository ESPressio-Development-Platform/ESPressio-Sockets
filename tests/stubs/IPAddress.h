#pragma once

#include <cstdint>

class IPAddress {
public:
    constexpr IPAddress() = default;
    constexpr IPAddress(uint8_t a, uint8_t b, uint8_t c, uint8_t d)
        : value_((static_cast<uint32_t>(a) << 24) |
                 (static_cast<uint32_t>(b) << 16) |
                 (static_cast<uint32_t>(c) << 8) |
                 static_cast<uint32_t>(d)) {}

    constexpr explicit operator uint32_t() const noexcept { return value_; }
    constexpr bool operator==(const IPAddress& other) const noexcept { return value_ == other.value_; }
    constexpr bool operator!=(const IPAddress& other) const noexcept { return value_ != other.value_; }

private:
    uint32_t value_ = 0;
};
