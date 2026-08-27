#pragma once

#include <string>

#include <ESPressio_Event.hpp>

namespace ESPressio::Event {

class SocketWorkerStartedEvent final : public TypedEvent<SocketWorkerStartedEvent> {
public:
    const std::string Name;
    explicit SocketWorkerStartedEvent(const char* name) : Name(name == nullptr ? "" : name) {}
};

class SocketWorkerStartFailedEvent final : public TypedEvent<SocketWorkerStartFailedEvent> {
public:
    const std::string Name;
    explicit SocketWorkerStartFailedEvent(const char* name) : Name(name == nullptr ? "" : name) {}
};

class SocketWorkerStoppedEvent final : public TypedEvent<SocketWorkerStoppedEvent> {};

} // namespace ESPressio::Event
