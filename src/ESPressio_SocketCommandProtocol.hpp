#pragma once

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <map>
#include <string>
#include <utility>
#include <vector>

#include "ESPressio_SocketCommandTypes.hpp"

namespace ESPressio::Sockets {

namespace SocketCommandProtocol {

constexpr uint32_t RequestMagic = 0x45534351UL;   // ESCQ
constexpr uint32_t ResponseMagic = 0x45534352UL;  // ESCR
constexpr uint8_t Version = 1;

namespace Detail {

inline void AppendU8(std::vector<uint8_t>& out, uint8_t value) {
    out.push_back(value);
}

inline void AppendU16(std::vector<uint8_t>& out, uint16_t value) {
    out.push_back(static_cast<uint8_t>((value >> 8) & 0xFF));
    out.push_back(static_cast<uint8_t>(value & 0xFF));
}

inline void AppendU32(std::vector<uint8_t>& out, uint32_t value) {
    for (int shift = 24; shift >= 0; shift -= 8) {
        out.push_back(static_cast<uint8_t>((value >> shift) & 0xFF));
    }
}

inline void AppendU64(std::vector<uint8_t>& out, uint64_t value) {
    for (int shift = 56; shift >= 0; shift -= 8) {
        out.push_back(static_cast<uint8_t>((value >> shift) & 0xFF));
    }
}

inline void AppendI32(std::vector<uint8_t>& out, int32_t value) {
    AppendU32(out, static_cast<uint32_t>(value));
}

inline bool AppendString16(std::vector<uint8_t>& out, const std::string& value) {
    if (value.size() > 0xFFFFU) return false;
    AppendU16(out, static_cast<uint16_t>(value.size()));
    out.insert(out.end(), value.begin(), value.end());
    return true;
}

inline bool AppendString32(std::vector<uint8_t>& out, const std::string& value) {
    if (value.size() > 0xFFFFFFFFULL) return false;
    AppendU32(out, static_cast<uint32_t>(value.size()));
    out.insert(out.end(), value.begin(), value.end());
    return true;
}

inline bool AppendCommandValue16(
    std::vector<uint8_t>& out,
    const Command::CommandValue& value
) {
    if (value.IsNull()) return false;
    return AppendString16(out, value.ToString());
}

class Reader {
public:
    Reader(const uint8_t* data, std::size_t size) : data_(data), size_(size) {}

    bool U8(uint8_t& value) {
        if (!Need(1)) return false;
        value = data_[offset_++];
        return true;
    }

    bool U16(uint16_t& value) {
        if (!Need(2)) return false;
        value = static_cast<uint16_t>((static_cast<uint16_t>(data_[offset_]) << 8) | data_[offset_ + 1]);
        offset_ += 2;
        return true;
    }

    bool U32(uint32_t& value) {
        if (!Need(4)) return false;
        value = 0;
        for (int i = 0; i < 4; ++i) value = (value << 8) | data_[offset_ + i];
        offset_ += 4;
        return true;
    }

    bool U64(uint64_t& value) {
        if (!Need(8)) return false;
        value = 0;
        for (int i = 0; i < 8; ++i) value = (value << 8) | data_[offset_ + i];
        offset_ += 8;
        return true;
    }

    bool I32(int32_t& value) {
        uint32_t raw = 0;
        if (!U32(raw)) return false;
        value = static_cast<int32_t>(raw);
        return true;
    }

    bool String16(std::string& value) {
        uint16_t length = 0;
        if (!U16(length) || !Need(length)) return false;
        value.assign(reinterpret_cast<const char*>(data_ + offset_), length);
        offset_ += length;
        return true;
    }

    bool String32(std::string& value) {
        uint32_t length = 0;
        if (!U32(length) || static_cast<uint64_t>(length) > Remaining()) return false;
        value.assign(reinterpret_cast<const char*>(data_ + offset_), length);
        offset_ += length;
        return true;
    }

    std::size_t Remaining() const { return size_ - offset_; }
    bool Finished() const { return offset_ == size_; }

private:
    bool Need(std::size_t count) const { return count <= size_ - offset_; }
    const uint8_t* data_ = nullptr;
    std::size_t size_ = 0;
    std::size_t offset_ = 0;
};

}

inline bool EncodeRequest(
    const SocketCommandInvocationContext& context,
    std::vector<uint8_t>& out
) {
    out.clear();
    Detail::AppendU32(out, RequestMagic);
    Detail::AppendU8(out, Version);
    Detail::AppendU64(out, context.Metadata.RequestID);

    if (context.Invocation.path.size() > 0xFFFFU ||
        context.Invocation.positional.size() > 0xFFFFU ||
        context.Invocation.named.size() > 0xFFFFU) return false;

    Detail::AppendU16(out, static_cast<uint16_t>(context.Invocation.path.size()));
    for (const auto& item : context.Invocation.path) {
        if (!Detail::AppendString16(out, item)) return false;
    }

    Detail::AppendU16(out, static_cast<uint16_t>(context.Invocation.positional.size()));
    for (const auto& item : context.Invocation.positional) {
        if (!Detail::AppendCommandValue16(out, item)) return false;
    }

    Detail::AppendU16(out, static_cast<uint16_t>(context.Invocation.named.size()));
    for (const auto& item : context.Invocation.named) {
        if (!Detail::AppendString16(out, item.first) ||
            !Detail::AppendCommandValue16(out, item.second)) return false;
    }

    if (!Detail::AppendString32(out, context.Invocation.raw)) return false;
    return true;
}

inline bool DecodeRequest(
    const uint8_t* data,
    std::size_t size,
    SocketCommandInvocationContext& context
) {
    if (data == nullptr) return false;
    Detail::Reader reader(data, size);
    uint32_t magic = 0;
    uint8_t version = 0;
    if (!reader.U32(magic) || magic != RequestMagic || !reader.U8(version) || version != Version) return false;

    context.Invocation = {};
    if (!reader.U64(context.Metadata.RequestID)) return false;

    uint16_t count = 0;
    if (!reader.U16(count)) return false;
    for (uint16_t i = 0; i < count; ++i) {
        std::string value;
        if (!reader.String16(value)) return false;
        context.Invocation.path.push_back(std::move(value));
    }

    if (!reader.U16(count)) return false;
    for (uint16_t i = 0; i < count; ++i) {
        std::string value;
        if (!reader.String16(value)) return false;
        context.Invocation.positional.emplace_back(std::move(value));
    }

    if (!reader.U16(count)) return false;
    for (uint16_t i = 0; i < count; ++i) {
        std::string key, value;
        if (!reader.String16(key) || !reader.String16(value)) return false;
        context.Invocation.named.emplace(
            std::move(key),
            Command::CommandValue(std::move(value))
        );
    }

    if (!reader.String32(context.Invocation.raw)) return false;
    return reader.Finished() && !context.Invocation.path.empty();
}

inline bool EncodeResponse(
    const SocketCommandResponse& response,
    std::vector<uint8_t>& out
) {
    out.clear();
    Detail::AppendU32(out, ResponseMagic);
    Detail::AppendU8(out, Version);
    Detail::AppendU64(out, response.RequestID);
    Detail::AppendU8(out, response.Result.success ? 1 : 0);
    Detail::AppendI32(out, static_cast<int32_t>(response.Result.code));
    return Detail::AppendString32(out, response.Result.message);
}

inline bool DecodeResponse(
    const uint8_t* data,
    std::size_t size,
    SocketCommandResponse& response
) {
    if (data == nullptr) return false;
    Detail::Reader reader(data, size);
    uint32_t magic = 0;
    uint8_t version = 0, success = 0;
    int32_t code = 0;
    if (!reader.U32(magic) || magic != ResponseMagic ||
        !reader.U8(version) || version != Version ||
        !reader.U64(response.RequestID) ||
        !reader.U8(success) || success > 1 ||
        !reader.I32(code) ||
        !reader.String32(response.Result.message) || !reader.Finished()) return false;
    response.Result.success = success != 0;
    response.Result.code = static_cast<int>(code);
    return true;
}

inline std::vector<uint8_t> FrameStructuredPayload(const std::vector<uint8_t>& payload) {
    if (payload.size() > 0xFFFFFFFFULL) return {};
    std::vector<uint8_t> out;
    out.reserve(payload.size() + 4);
    Detail::AppendU32(out, static_cast<uint32_t>(payload.size()));
    out.insert(out.end(), payload.begin(), payload.end());
    return out;
}

}

}
