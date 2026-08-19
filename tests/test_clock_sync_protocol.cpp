#include <cassert>
#include <cstdint>

#include <ESPressio_SocketClockSynchronizationProtocol.hpp>

using namespace ESPressio;

class FakeTarget final :
    public Timing::IClockSynchronizationTarget<Timing::ClockTick> {
public:
    uint64_t Now = 1000;
    Timing::ClockSynchronizationSample<Timing::ClockTick> LastSample;
    bool Submitted = false;

    uint64_t GetSynchronizationTimestampNanoseconds() const override {
        return Now;
    }

    Timing::ClockSynchronizationResult<Timing::ClockTick>
    SubmitSynchronizationSample(
        const Timing::ClockSynchronizationSample<Timing::ClockTick>& sample,
        Timing::ClockSynchronizationAdjustmentMode
    ) override {
        LastSample = sample;
        Submitted = true;
        Timing::ClockSynchronizationResult<Timing::ClockTick> result;
        result.Accepted = true;
        return result;
    }

    Timing::ClockSynchronizationStatus<Timing::ClockTick>
    GetSynchronizationStatus() const override { return {}; }

    void ConfigureSynchronization(
        const Timing::ClockSynchronizationConfig&
    ) override {}

    Timing::ClockSynchronizationConfig
    GetSynchronizationConfig() const override { return {}; }

    void ResetSynchronization() override {}
};

int main() {
    FakeTarget clientTarget;
    FakeTarget referenceTarget;

    Sockets::SocketClockSynchronizationProtocol client(&clientTarget);
    Sockets::SocketClockSynchronizationProtocol reference(&referenceTarget);

    Sockets::SocketClockSynchronizationConfig clientConfig;
    clientConfig.Mode = Sockets::SocketClockSynchronizationMode::Client;
    client.Configure(clientConfig);

    Sockets::SocketClockSynchronizationConfig referenceConfig;
    referenceConfig.Mode = Sockets::SocketClockSynchronizationMode::Reference;
    reference.Configure(referenceConfig);

    clientTarget.Now = 1000;
    Sockets::SocketClockSynchronizationProtocol::RequestMessage request;
    assert(client.BuildRequest(request));
    assert(request.T1 == 1000);

    referenceTarget.Now = 1600;
    Sockets::SocketClockSynchronizationProtocol::ResponseMessage response;
    bool gotResponse = false;

    assert(reference.ProcessRequest(
        reinterpret_cast<const uint8_t*>(&request),
        sizeof(request),
        1500,
        [&](const uint8_t* data, std::size_t size) {
            assert(size == sizeof(response));
            std::memcpy(&response, data, size);
            gotResponse = true;
            return true;
        }
    ));

    assert(gotResponse);
    assert(response.T1 == 1000);
    assert(response.T2 == 1500);
    assert(response.T3 == 1600);

    assert(client.ProcessResponse(
        reinterpret_cast<const uint8_t*>(&response),
        sizeof(response),
        1200
    ));

    assert(clientTarget.Submitted);
    assert(clientTarget.LastSample.LocalRequestTransmitTime == 1000);
    assert(clientTarget.LastSample.RemoteRequestReceiveTime == 1500);
    assert(clientTarget.LastSample.RemoteResponseTransmitTime == 1600);
    assert(clientTarget.LastSample.LocalResponseReceiveTime == 1200);

    return 0;
}
