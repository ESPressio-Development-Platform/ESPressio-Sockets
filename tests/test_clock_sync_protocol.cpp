#include <ESPressio_SocketClockSynchronizationProtocol.hpp>

#include <array>
#include <cassert>
#include <cstdint>
#include <vector>

using namespace ESPressio;

class FakeTarget final : public Timing::IClockSynchronizationTarget {
public:
    Timing::ClockTimestampCapture<> NextCapture{};
    Timing::ClockSynchronizationObservation<> LastObservation{};
    Timing::ClockSynchronizationStatus Status{};
    Timing::ClockSynchronizationProfile Profile{};
    std::uint64_t SelectedReference=0;
    std::size_t SubmitCount=0;
    std::size_t ResetCount=0;
    std::size_t DeadlineMisses=0;
    bool Acquiring=false;
    bool ReferenceAvailable=false;

    Timing::ClockTimestampCapture<> CaptureSynchronizationTimestamp(
        Timing::ClockCaptureQuality quality,Timing::ClockUncertainty uncertainty) const override {
        auto result=NextCapture;
        result.Quality=quality;
        result.Uncertainty=uncertainty;
        return result;
    }
    Timing::ClockSynchronizationResult SubmitSynchronizationObservation(
        const Timing::ClockSynchronizationObservation<>& observation) override {
        LastObservation=observation;
        ++SubmitCount;
        Timing::ClockSynchronizationResult result{};
        result.Accepted=true;
        return result;
    }
    Timing::ClockSynchronizationStatus GetSynchronizationStatus() const override { return Status; }
    Timing::ClockConfigurationStatus ConfigureSynchronization(const Timing::ClockSynchronizationProfile& profile) override {
        Profile=profile; return Timing::ClockConfigurationStatus::Success;
    }
    Timing::ClockSynchronizationProfile GetSynchronizationProfile() const override { return Profile; }
    Timing::ClockConfigurationStatus SelectSynchronizationReference(std::uint64_t reference) override {
        SelectedReference=reference; return reference==0 ? Timing::ClockConfigurationStatus::InvalidReference : Timing::ClockConfigurationStatus::Success;
    }
    void SetSynchronizationActivity(bool acquiring,bool available) override {
        Acquiring=acquiring; ReferenceAvailable=available;
    }
    void ResetSynchronization() override { ++ResetCount; }
    void RecordSynchronizationDeadlineMiss() override { ++DeadlineMisses; }
};

struct Sink final {
    std::vector<std::uint8_t> Bytes;
    static bool Send(void* owner,const std::uint8_t* data,std::size_t size) noexcept {
        auto& self=*static_cast<Sink*>(owner);
        self.Bytes.assign(data,data+size);
        return true;
    }
};

int main() {
    using Protocol=Sockets::SocketClockSynchronizationProtocol;
    using Wire=Sockets::SocketClockWireV2;

    FakeTarget clientTarget,referenceTarget;
    Protocol client(&clientTarget),reference(&referenceTarget);

    Sockets::SocketClockSynchronizationConfig clientConfig{};
    clientConfig.Mode=Sockets::SocketClockSynchronizationMode::Client;
    clientConfig.ReferenceIdentity=42;
    clientConfig.RequestTimeoutNanoseconds=500;
    clientConfig.LocalTransmitCaptureQuality=Timing::ClockCaptureQuality::SoftwareBounded;
    clientConfig.LocalTransmitCaptureUncertainty=Timing::ClockUncertainty::Known(10);
    assert(client.Configure(clientConfig));
    assert(clientTarget.SelectedReference==42);
    assert(clientTarget.Acquiring&&!clientTarget.ReferenceAvailable);

    Sockets::SocketClockSynchronizationConfig referenceConfig{};
    referenceConfig.Mode=Sockets::SocketClockSynchronizationMode::Reference;
    referenceConfig.LocalTransmitCaptureQuality=Timing::ClockCaptureQuality::SoftwareBounded;
    referenceConfig.LocalTransmitCaptureUncertainty=Timing::ClockUncertainty::Known(20);
    assert(reference.Configure(referenceConfig));

    referenceTarget.Status.Reliability=Timing::TimeReliability::Synchronized;
    referenceTarget.Status.CurrentUncertainty=Timing::ClockUncertainty::Known(100);

    client.SetReferenceAvailable(true);
    assert(clientTarget.ReferenceAvailable);
    clientTarget.NextCapture.SystemTimeNanoseconds=1000;
    clientTarget.NextCapture.MonotonicTimeNanoseconds=100;

    std::array<std::uint8_t,Wire::RequestBytes> request{};
    std::size_t requestBytes=0;
    assert(client.BuildRequest(request.data(),request.size(),requestBytes));
    assert(requestBytes==Wire::RequestBytes);
    assert(Wire::Read32(request.data())==Wire::Magic);
    assert(request[4]==Wire::Version);
    assert(request[5]==static_cast<std::uint8_t>(Wire::Kind::Request));
    const auto sequence=Wire::Read32(request.data()+8);
    assert(sequence!=0 && client.PendingSequence()==sequence);
    std::size_t ignored=0;
    assert(!client.BuildRequest(request.data(),request.size(),ignored));

    Timing::ClockTimestampCapture<> t2{};
    t2.SystemTimeNanoseconds=1500;
    t2.MonotonicTimeNanoseconds=500;
    t2.Quality=Timing::ClockCaptureQuality::Hardware;
    t2.Uncertainty=Timing::ClockUncertainty::Known(5);
    referenceTarget.NextCapture.SystemTimeNanoseconds=1600;
    referenceTarget.NextCapture.MonotonicTimeNanoseconds=600;

    Sink sink;
    assert(reference.ProcessRequest(request.data(),requestBytes,t2,{&sink,&Sink::Send}));
    assert(sink.Bytes.size()==Wire::ResponseBytes);
    assert(sink.Bytes[5]==static_cast<std::uint8_t>(Wire::Kind::Response));
    assert(Wire::Read32(sink.Bytes.data()+8)==sequence);

    Wire::Response decoded{};
    assert(Wire::DecodeResponse(sink.Bytes.data(),sink.Bytes.size(),decoded));
    assert(decoded.T2.SystemTimeNanoseconds==1500);
    assert(decoded.T2.MonotonicTimeNanoseconds==500);
    assert(decoded.T2.Quality==Timing::ClockCaptureQuality::Hardware);
    assert(decoded.T2.Uncertainty.IsKnown&&decoded.T2.Uncertainty.Nanoseconds==5);
    assert(decoded.T3.SystemTimeNanoseconds==1600);
    assert(decoded.T3.MonotonicTimeNanoseconds==600);
    assert(decoded.T3.Quality==Timing::ClockCaptureQuality::SoftwareBounded);
    assert(decoded.T3.Uncertainty.IsKnown&&decoded.T3.Uncertainty.Nanoseconds==20);
    assert(decoded.ReferenceReliability==Timing::TimeReliability::Synchronized);
    assert(decoded.ReferenceUncertainty.IsKnown&&decoded.ReferenceUncertainty.Nanoseconds==100);

    Timing::ClockTimestampCapture<> t4{};
    t4.SystemTimeNanoseconds=1200;
    t4.MonotonicTimeNanoseconds=300;
    t4.Quality=Timing::ClockCaptureQuality::SoftwareBounded;
    t4.Uncertainty=Timing::ClockUncertainty::Known(10);
    assert(client.ProcessResponse(sink.Bytes.data(),sink.Bytes.size(),t4));
    assert(client.PendingSequence()==0);
    assert(clientTarget.SubmitCount==1);
    assert(clientTarget.LastObservation.T1.SystemTimeNanoseconds==1000);
    assert(clientTarget.LastObservation.T1.MonotonicTimeNanoseconds==100);
    assert(clientTarget.LastObservation.T2.SystemTimeNanoseconds==1500);
    assert(clientTarget.LastObservation.T3.SystemTimeNanoseconds==1600);
    assert(clientTarget.LastObservation.T4.SystemTimeNanoseconds==1200);
    assert(clientTarget.LastObservation.ReferenceIdentity==42);
    assert(clientTarget.LastObservation.ReferenceReliability==Timing::TimeReliability::Synchronized);
    assert(clientTarget.LastObservation.ReferenceUncertainty.Nanoseconds==100);

    clientTarget.Status.HasSynchronizationDeadline=true;
    clientTarget.Status.NextRequiredSynchronizationMonotonic=1000;
    assert(!client.EvidenceDue(999));
    assert(client.EvidenceDue(1000));

    clientTarget.NextCapture.SystemTimeNanoseconds=2000;
    clientTarget.NextCapture.MonotonicTimeNanoseconds=2000;
    assert(client.BuildRequest(request.data(),request.size(),requestBytes));
    const auto pending=client.PendingSequence();
    auto stale=sink.Bytes;
    Wire::Write32(stale.data()+8,pending+1);
    assert(!client.ProcessResponse(stale.data(),stale.size(),t4));
    assert(client.PendingSequence()==pending);
    assert(!client.ServiceTimeout(2499));
    assert(client.ServiceTimeout(2500));
    assert(client.PendingSequence()==0);
    assert(clientTarget.DeadlineMisses==1);

    clientTarget.NextCapture.SystemTimeNanoseconds=3000;
    clientTarget.NextCapture.MonotonicTimeNanoseconds=3000;
    assert(client.BuildRequest(request.data(),request.size(),requestBytes));
    client.NotifyReferenceContinuityLost();
    assert(client.PendingSequence()==0);
    assert(clientTarget.ResetCount==1);

    client.SetReferenceAvailable(false);
    assert(!clientTarget.ReferenceAvailable);
    assert(!client.EvidenceDue(5000));

    Sockets::SocketClockSynchronizationConfig invalid=clientConfig;
    invalid.ReferenceIdentity=0;
    Protocol invalidClient(&clientTarget);
    assert(!invalidClient.Configure(invalid));

    auto malformed=sink.Bytes;
    malformed[44]=0;
    assert(!Wire::DecodeResponse(malformed.data(),malformed.size(),decoded));
    return 0;
}
