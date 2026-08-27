#include <cassert>
#include <cstdint>
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

} // namespace

int main() {
    using namespace ESPressio;

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

    assert(sessionA.Initialize(
        publisherA,
        remoteA,
        subscriptionsA,
        config,
        [&](const uint8_t* data, std::size_t size) {
            return sessionB.Feed(data, size);
        }
    ));
    assert(sessionB.Initialize(
        publisherB,
        remoteB,
        subscriptionsB,
        config,
        [&](const uint8_t* data, std::size_t size) {
            return sessionA.Feed(data, size);
        }
    ));

    // B requests A's Temperature. The subscription control binds both peers,
    // triggers an authoritative snapshot, and ACKs the accepted revision.
    assert(subscriptionsB.Subscribe<Temperature>());
    assert(sessionA.RemoteDevice() == deviceB);
    assert(sessionB.RemoteDevice() == deviceA);

    State::RemoteStateSnapshot<int32_t> temperatureSnapshot;
    assert(remoteB.Read<Temperature>(deviceA, temperatureSnapshot));
    assert(temperatureSnapshot.HasValue);
    assert(temperatureSnapshot.Value == 21);

    // Subsequent publication traverses the same session without polling.
    temperatureA = 27;
    assert(publisherA.Publish<Temperature>());
    assert(remoteB.Read<Temperature>(deviceA, temperatureSnapshot));
    assert(temperatureSnapshot.Value == 27);

    // The connection is fully bidirectional: A can subscribe to B independently.
    assert(subscriptionsA.Subscribe<Enabled>());
    State::RemoteStateSnapshot<bool> enabledSnapshot;
    assert(remoteA.Read<Enabled>(deviceB, enabledSnapshot));
    assert(enabledSnapshot.HasValue);
    assert(enabledSnapshot.Value == false);

    enabledB = true;
    assert(publisherB.Publish<Enabled>());
    assert(remoteA.Read<Enabled>(deviceB, enabledSnapshot));
    assert(enabledSnapshot.Value == true);

    // Unsubscribe prevents later values from mutating the remote repository.
    assert(subscriptionsB.Unsubscribe<Temperature>());
    temperatureA = 41;
    assert(publisherA.Publish<Temperature>());
    assert(remoteB.Read<Temperature>(deviceA, temperatureSnapshot));
    assert(temperatureSnapshot.Value == 27);

    // Framing supports fragmented TCP reads.
    std::vector<uint8_t> delivered;
    Sockets::SocketStateFrameDecoder decoder(64);
    const uint8_t protocol[] = {1, 2, 3, 4, 5};
    auto frame = Sockets::BuildSocketStateFrame(protocol, sizeof(protocol), 64);
    assert(!frame.empty());
    assert(decoder.Push(frame.data(), 3, [&](const uint8_t*, std::size_t) { assert(false); }));
    assert(decoder.Push(
        frame.data() + 3,
        frame.size() - 3,
        [&](const uint8_t* data, std::size_t size) {
            delivered.assign(data, data + size);
        }
    ));
    assert(delivered == std::vector<uint8_t>(protocol, protocol + sizeof(protocol)));

    sessionA.Shutdown();
    sessionB.Shutdown();
    return 0;
}
