#include <cassert>
#include <cstdint>
#include <cstdio>
#include <vector>

#include <ESPressio_State.hpp>

#include "ESPressio_SocketStateSession.hpp"

namespace {

struct Temperature {
    using Value = int32_t;
    static constexpr ESPressio::State::StateTypeId Id = 0x53544354454D5001ULL;
};

struct Enabled {
    using Value = bool;
    static constexpr ESPressio::State::StateTypeId Id = 0x535443454E414201ULL;
};

using Contract = ESPressio::State::StateContract<Temperature, Enabled>;
using Publisher = ESPressio::State::StatePublisher<Contract>;
using Remote = ESPressio::State::RemoteStateManager<Contract, 2>;
using Subscriptions = ESPressio::State::StateSubscriptionRegistry<4>;
using Session = ESPressio::Sockets::SocketStateSession<Contract, 2, 4>;

ESPressio::State::DeviceIdentifier MakeDevice(uint8_t tail) {
    ESPressio::State::DeviceIdentifier::Storage bytes{};
    bytes[15] = tail;
    return ESPressio::State::DeviceIdentifier(bytes);
}

void Checkpoint(const char* text) {
    std::fprintf(stderr, "%s\n", text);
    std::fflush(stderr);
}

void VerifyReentrantFrameDelivery() {
    using namespace ESPressio::Sockets;

    SocketStateFrameDecoder decoder(64);
    const uint8_t firstPayload[] = {0x11};
    const uint8_t secondPayload[] = {0x22};
    const auto firstFrame = BuildSocketStateFrame(firstPayload, sizeof(firstPayload), 64);
    const auto secondFrame = BuildSocketStateFrame(secondPayload, sizeof(secondPayload), 64);
    assert(!firstFrame.empty());
    assert(!secondFrame.empty());

    std::vector<uint8_t> delivered;
    assert(decoder.Push(
        firstFrame.data(),
        firstFrame.size(),
        [&](const uint8_t* payload, std::size_t size) {
            assert(size == 1);
            delivered.push_back(payload[0]);
            assert(decoder.Push(
                secondFrame.data(),
                secondFrame.size(),
                [&](const uint8_t* nestedPayload, std::size_t nestedSize) {
                    assert(nestedSize == 1);
                    delivered.push_back(nestedPayload[0]);
                }
            ));
        }
    ));

    assert(delivered.size() == 2);
    assert(delivered[0] == firstPayload[0]);
    assert(delivered[1] == secondPayload[0]);
}

} // namespace

int main() {
    using namespace ESPressio;

    VerifyReentrantFrameDelivery();

    int32_t temperatureA = 21;
    int32_t temperatureB = 32;
    bool enabledA = true;
    bool enabledB = false;
    const auto deviceA = MakeDevice(0xA1);
    const auto deviceB = MakeDevice(0xB2);

    Publisher publisherA(deviceA);
    Publisher publisherB(deviceB);
    assert(publisherA.RegisterSource<Temperature>([&] { return temperatureA; }));
    assert(publisherA.RegisterSource<Enabled>([&] { return enabledA; }));
    assert(publisherB.RegisterSource<Temperature>([&] { return temperatureB; }));
    assert(publisherB.RegisterSource<Enabled>([&] { return enabledB; }));

    Remote remoteA;
    Remote remoteB;
    Subscriptions subscriptionsA;
    Subscriptions subscriptionsB;
    Session sessionA;
    Session sessionB;
    Sockets::SocketStateSessionConfig config;
    config.MaximumProtocolMessageBytes = 256;

    Checkpoint("initialize A");
    assert(sessionA.Initialize(
        publisherA,
        remoteA,
        subscriptionsA,
        config,
        [&](const uint8_t* data, std::size_t size) {
            return sessionB.Feed(data, size);
        }
    ));

    Checkpoint("initialize B");
    assert(sessionB.Initialize(
        publisherB,
        remoteB,
        subscriptionsB,
        config,
        [&](const uint8_t* data, std::size_t size) {
            return sessionA.Feed(data, size);
        }
    ));

    Checkpoint("subscribe B->A temperature");
    assert(subscriptionsB.Subscribe<Temperature>());
    Checkpoint("subscribed B->A temperature");
    assert(sessionA.RemoteDevice() == deviceB);
    assert(sessionB.RemoteDevice() == deviceA);

    State::RemoteStateSnapshot<int32_t> temperatureSnapshot;
    assert(remoteB.Read<Temperature>(deviceA, temperatureSnapshot));
    assert(temperatureSnapshot.HasValue);
    assert(temperatureSnapshot.Value == 21);

    temperatureA = 27;
    Checkpoint("publish A temperature");
    assert(publisherA.Publish<Temperature>());
    assert(remoteB.Read<Temperature>(deviceA, temperatureSnapshot));
    assert(temperatureSnapshot.Value == 27);

    Checkpoint("subscribe A->B enabled");
    assert(subscriptionsA.Subscribe<Enabled>());
    State::RemoteStateSnapshot<bool> enabledSnapshot;
    assert(remoteA.Read<Enabled>(deviceB, enabledSnapshot));
    assert(enabledSnapshot.HasValue);
    assert(!enabledSnapshot.Value);

    enabledB = true;
    Checkpoint("publish B enabled");
    assert(publisherB.Publish<Enabled>());
    assert(remoteA.Read<Enabled>(deviceB, enabledSnapshot));
    assert(enabledSnapshot.Value);

    Checkpoint("unsubscribe B temperature");
    assert(subscriptionsB.Unsubscribe<Temperature>());
    temperatureA = 41;
    assert(publisherA.Publish<Temperature>());
    assert(remoteB.Read<Temperature>(deviceA, temperatureSnapshot));
    assert(temperatureSnapshot.Value == 27);

    std::vector<uint8_t> delivered;
    Sockets::SocketStateFrameDecoder decoder(64);
    const uint8_t protocol[] = {1, 2, 3, 4, 5};
    const auto frame = Sockets::BuildSocketStateFrame(protocol, sizeof(protocol), 64);
    assert(!frame.empty());
    assert(decoder.Push(
        frame.data(),
        3,
        [&](const uint8_t*, std::size_t) {
            assert(false);
        }
    ));
    assert(decoder.Push(
        frame.data() + 3,
        frame.size() - 3,
        [&](const uint8_t* data, std::size_t size) {
            delivered.assign(data, data + size);
        }
    ));
    assert(delivered == std::vector<uint8_t>(protocol, protocol + sizeof(protocol)));

    Checkpoint("shutdown A");
    sessionA.Shutdown();
    Checkpoint("shutdown B");
    sessionB.Shutdown();
    Checkpoint("done");
    return 0;
}
