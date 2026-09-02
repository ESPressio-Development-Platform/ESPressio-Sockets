#pragma once

#include <ESPressio_Event.hpp>
#include <ESPressio_SecurityTypes.hpp>

namespace ESPressio::Event {

class SocketSecuritySessionFaultedEvent final : public TypedEvent<SocketSecuritySessionFaultedEvent> {
public:
    const Security::SecurityResult Result;
    explicit SocketSecuritySessionFaultedEvent(const Security::SecurityResult& result) : Result(result) {}
};

class SocketSecuritySessionResetEvent final : public TypedEvent<SocketSecuritySessionResetEvent> {};

} // namespace ESPressio::Event
