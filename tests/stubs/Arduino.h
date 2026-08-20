#pragma once

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <string>

using String = std::string;

inline uint32_t millis() { return 1234; }

class Print {
public:
    virtual ~Print() = default;
    virtual std::size_t write(uint8_t value) = 0;
    virtual std::size_t write(const uint8_t* data, std::size_t size) {
        std::size_t written = 0;
        for (std::size_t i = 0; i < size; ++i) written += write(data[i]);
        return written;
    }
    std::size_t print(const char* value) {
        if (value == nullptr) return 0;
        return write(reinterpret_cast<const uint8_t*>(value), std::strlen(value));
    }
    std::size_t print(const std::string& value) {
        return write(reinterpret_cast<const uint8_t*>(value.data()), value.size());
    }
    std::size_t print(char value) { return write(static_cast<uint8_t>(value)); }
    std::size_t print(int value) { return print(std::to_string(value)); }
    std::size_t print(unsigned int value) { return print(std::to_string(value)); }
    std::size_t print(long value) { return print(std::to_string(value)); }
    std::size_t print(unsigned long value) { return print(std::to_string(value)); }
    std::size_t println() { return print("\n"); }
    template<typename TValue> std::size_t println(const TValue& value) { return print(value) + println(); }
};

class Stream : public Print {
public:
    virtual int available() = 0;
    virtual int read() = 0;
};
