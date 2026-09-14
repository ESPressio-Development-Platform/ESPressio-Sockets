#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

#include <ESPressio_IClockSynchronizationTarget.hpp>
#include <ESPressio_SystemClock.hpp>

#include "ESPressio_SocketClockSynchronizationTypes.hpp"

namespace ESPressio::Sockets {

/// <summary>Endian-stable bounded wire codec for direct socket clock exchanges.</summary>
struct SocketClockWireV2 final {
    static constexpr std::uint32_t Magic=0x53434C4Bu; // SCLK
    static constexpr std::uint8_t Version=2;
    static constexpr std::size_t HeaderBytes=12;
    static constexpr std::size_t RequestBytes=HeaderBytes;
    static constexpr std::size_t ResponseBytes=72;

    enum class Kind : std::uint8_t { Request=1,Response=2 };

    struct Header final { Kind Type{Kind::Request}; std::uint32_t Sequence{0}; };
    struct Response final {
        Header Common{Kind::Response,0};
        Timing::ClockTimestampCapture<> T2{};
        Timing::ClockTimestampCapture<> T3{};
        Timing::TimeReliability ReferenceReliability{Timing::TimeReliability::Unqualified};
        Timing::ClockUncertainty ReferenceUncertainty{};
    };

    static void Write16(std::uint8_t* out,std::uint16_t value) noexcept {
        out[0]=static_cast<std::uint8_t>(value);
        out[1]=static_cast<std::uint8_t>(value>>8);
    }
    static void Write32(std::uint8_t* out,std::uint32_t value) noexcept {
        for(std::size_t i=0;i<4;++i) out[i]=static_cast<std::uint8_t>(value>>(i*8));
    }
    static void Write64(std::uint8_t* out,std::uint64_t value) noexcept {
        for(std::size_t i=0;i<8;++i) out[i]=static_cast<std::uint8_t>(value>>(i*8));
    }
    static std::uint16_t Read16(const std::uint8_t* in) noexcept {
        return static_cast<std::uint16_t>(in[0]) |
               static_cast<std::uint16_t>(static_cast<std::uint16_t>(in[1])<<8);
    }
    static std::uint32_t Read32(const std::uint8_t* in) noexcept {
        std::uint32_t value=0;
        for(std::size_t i=0;i<4;++i) value|=static_cast<std::uint32_t>(in[i])<<(i*8);
        return value;
    }
    static std::uint64_t Read64(const std::uint8_t* in) noexcept {
        std::uint64_t value=0;
        for(std::size_t i=0;i<8;++i) value|=static_cast<std::uint64_t>(in[i])<<(i*8);
        return value;
    }

    static bool ValidQuality(Timing::ClockCaptureQuality quality) noexcept {
        return quality==Timing::ClockCaptureQuality::Hardware ||
               quality==Timing::ClockCaptureQuality::SoftwareBounded ||
               quality==Timing::ClockCaptureQuality::SoftwareUnbounded;
    }

    static bool EncodeHeader(Header header,std::uint8_t* out,std::size_t capacity) noexcept {
        if(out==nullptr || capacity<HeaderBytes || header.Sequence==0 ||
           (header.Type!=Kind::Request && header.Type!=Kind::Response)) return false;
        Write32(out,Magic);
        out[4]=Version;
        out[5]=static_cast<std::uint8_t>(header.Type);
        Write16(out+6,0);
        Write32(out+8,header.Sequence);
        return true;
    }

    static bool DecodeHeader(const std::uint8_t* data,std::size_t size,Header& out) noexcept {
        if(data==nullptr || size<HeaderBytes || Read32(data)!=Magic || data[4]!=Version || Read16(data+6)!=0)
            return false;
        const auto kind=static_cast<Kind>(data[5]);
        const auto sequence=Read32(data+8);
        if(sequence==0 || (kind!=Kind::Request && kind!=Kind::Response)) return false;
        out={kind,sequence};
        return true;
    }

    static bool EncodeRequest(std::uint32_t sequence,std::uint8_t* out,std::size_t capacity) noexcept {
        return capacity>=RequestBytes && EncodeHeader({Kind::Request,sequence},out,capacity);
    }

    static bool DecodeRequest(const std::uint8_t* data,std::size_t size,std::uint32_t& sequence) noexcept {
        Header header{};
        if(size!=RequestBytes || !DecodeHeader(data,size,header) || header.Type!=Kind::Request) return false;
        sequence=header.Sequence;
        return true;
    }

    static bool EncodeResponse(const Response& response,std::uint8_t* out,std::size_t capacity) noexcept {
        if(capacity<ResponseBytes || !EncodeHeader(response.Common,out,capacity) || response.Common.Type!=Kind::Response ||
           !ValidQuality(response.T2.Quality) || !ValidQuality(response.T3.Quality) ||
           !Timing::IsValidTimeReliability(response.ReferenceReliability)) return false;
        Write64(out+12,response.T2.SystemTimeNanoseconds);
        Write64(out+20,response.T2.MonotonicTimeNanoseconds);
        Write64(out+28,response.T3.SystemTimeNanoseconds);
        Write64(out+36,response.T3.MonotonicTimeNanoseconds);
        out[44]=static_cast<std::uint8_t>(response.T2.Quality);
        out[45]=static_cast<std::uint8_t>(response.T3.Quality);
        out[46]=static_cast<std::uint8_t>(response.ReferenceReliability);
        std::uint8_t flags=0;
        if(response.T2.Uncertainty.IsKnown) flags|=0x01u;
        if(response.T3.Uncertainty.IsKnown) flags|=0x02u;
        if(response.ReferenceUncertainty.IsKnown) flags|=0x04u;
        out[47]=flags;
        Write64(out+48,response.T2.Uncertainty.Nanoseconds);
        Write64(out+56,response.T3.Uncertainty.Nanoseconds);
        Write64(out+64,response.ReferenceUncertainty.Nanoseconds);
        return true;
    }

    static bool DecodeResponse(const std::uint8_t* data,std::size_t size,Response& out) noexcept {
        Header header{};
        if(size!=ResponseBytes || !DecodeHeader(data,size,header) || header.Type!=Kind::Response) return false;
        const auto t2Quality=static_cast<Timing::ClockCaptureQuality>(data[44]);
        const auto t3Quality=static_cast<Timing::ClockCaptureQuality>(data[45]);
        const auto reliability=static_cast<Timing::TimeReliability>(data[46]);
        const auto flags=data[47];
        if((flags&0xF8u)!=0 || !ValidQuality(t2Quality) || !ValidQuality(t3Quality) ||
           !Timing::IsValidTimeReliability(reliability)) return false;
        out={};
        out.Common=header;
        out.T2.SystemTimeNanoseconds=Read64(data+12);
        out.T2.MonotonicTimeNanoseconds=Read64(data+20);
        out.T3.SystemTimeNanoseconds=Read64(data+28);
        out.T3.MonotonicTimeNanoseconds=Read64(data+36);
        out.T2.Quality=t2Quality;
        out.T3.Quality=t3Quality;
        out.ReferenceReliability=reliability;
        if((flags&0x01u)!=0) out.T2.Uncertainty=Timing::ClockUncertainty::Known(Read64(data+48));
        if((flags&0x02u)!=0) out.T3.Uncertainty=Timing::ClockUncertainty::Known(Read64(data+56));
        if((flags&0x04u)!=0) out.ReferenceUncertainty=Timing::ClockUncertainty::Known(Read64(data+64));
        return true;
    }
};

/// <summary>One-bounded-exchange K1/K2 transport seam for socket/network providers.</summary>
/// <remarks>
/// This object owns no worker, estimator, fixed cadence or System reconstruction. A concrete socket provider supplies
/// provider-proximate T2/T4 capture evidence and drives EvidenceDue() from its existing bounded service context.
/// Timing alone validates the resulting four-capture observation and owns reliability, uncertainty and discipline.
/// </remarks>
class SocketClockSynchronizationProtocol final {
    Timing::IClockSynchronizationTarget* _target{nullptr};
    SocketClockSynchronizationConfig _config{};
    std::uint32_t _nextSequence{1};
    std::uint32_t _pendingSequence{0};
    Timing::ClockTimestampCapture<> _pendingT1{};
    bool _referenceAvailable{false};
    bool _configured{false};

    static bool IsClientMode(SocketClockSynchronizationMode mode) noexcept {
        return mode==SocketClockSynchronizationMode::Client || mode==SocketClockSynchronizationMode::ClientAndReference;
    }
    static bool IsReferenceMode(SocketClockSynchronizationMode mode) noexcept {
        return mode==SocketClockSynchronizationMode::Reference || mode==SocketClockSynchronizationMode::ClientAndReference;
    }
    static bool ValidLocalCapture(const SocketClockSynchronizationConfig& config) noexcept {
        if(config.LocalTransmitCaptureQuality==Timing::ClockCaptureQuality::SoftwareUnbounded)
            return !config.LocalTransmitCaptureUncertainty.IsKnown;
        if(config.LocalTransmitCaptureQuality==Timing::ClockCaptureQuality::SoftwareBounded)
            return config.LocalTransmitCaptureUncertainty.IsKnown;
        return false;
    }
    static bool ValidCapture(const Timing::ClockTimestampCapture<>& capture) noexcept {
        return SocketClockWireV2::ValidQuality(capture.Quality) && capture.MonotonicTimeNanoseconds!=0;
    }

public:
    explicit SocketClockSynchronizationProtocol(Timing::IClockSynchronizationTarget* target=nullptr) noexcept
        :_target(target!=nullptr?target:static_cast<Timing::IClockSynchronizationTarget*>(&Timing::SystemClock<>::GetInstance())) {}

    bool Configure(const SocketClockSynchronizationConfig& config) noexcept {
        if(_target==nullptr || config.RequestTimeoutNanoseconds==0 || !ValidLocalCapture(config) ||
           (IsClientMode(config.Mode) && config.ReferenceIdentity==0)) return false;
        if(IsClientMode(config.Mode) &&
           _target->SelectSynchronizationReference(config.ReferenceIdentity)!=Timing::ClockConfigurationStatus::Success)
            return false;
        CancelPendingRequest();
        _config=config;
        _referenceAvailable=false;
        _configured=true;
        if(IsClientMode(_config.Mode)) _target->SetSynchronizationActivity(true,false);
        return true;
    }

    const SocketClockSynchronizationConfig& GetConfig() const noexcept { return _config; }
    bool IsConfigured() const noexcept { return _configured; }
    bool IsClient() const noexcept { return _configured && IsClientMode(_config.Mode); }
    bool IsReference() const noexcept { return _configured && IsReferenceMode(_config.Mode); }
    std::uint32_t PendingSequence() const noexcept { return _pendingSequence; }

    void SetReferenceAvailable(bool available) noexcept {
        _referenceAvailable=available;
        if(IsClient()) {
            if(!available) CancelPendingRequest();
            _target->SetSynchronizationActivity(true,available);
        }
    }

    void NotifyReferenceContinuityLost() noexcept {
        CancelPendingRequest();
        if(IsClient()) _target->ResetSynchronization();
    }

    Timing::ClockTimestampCapture<> CaptureServiceReceive() const noexcept {
        return _target==nullptr ? Timing::ClockTimestampCapture<>{} :
            _target->CaptureSynchronizationTimestamp(Timing::ClockCaptureQuality::SoftwareUnbounded,{});
    }

    bool EvidenceDue(std::uint64_t nowMonotonicNanoseconds) const noexcept {
        if(!IsClient() || !_referenceAvailable || _pendingSequence!=0 || nowMonotonicNanoseconds==0) return false;
        const auto status=_target->GetSynchronizationStatus();
        return status.HasSynchronizationDeadline && status.NextRequiredSynchronizationMonotonic!=0 &&
               nowMonotonicNanoseconds>=status.NextRequiredSynchronizationMonotonic;
    }

    bool ServiceTimeout(std::uint64_t nowMonotonicNanoseconds) noexcept {
        if(_pendingSequence==0 || _pendingT1.MonotonicTimeNanoseconds==0 || nowMonotonicNanoseconds<_pendingT1.MonotonicTimeNanoseconds)
            return false;
        if(nowMonotonicNanoseconds-_pendingT1.MonotonicTimeNanoseconds<_config.RequestTimeoutNanoseconds) return false;
        CancelPendingRequest();
        _target->RecordSynchronizationDeadlineMiss();
        return true;
    }

    bool BuildRequest(std::uint8_t* output,std::size_t capacity,std::size_t& bytes) noexcept {
        bytes=0;
        if(!IsClient() || !_referenceAvailable || _pendingSequence!=0 || output==nullptr || capacity<SocketClockWireV2::RequestBytes)
            return false;
        auto t1=_target->CaptureSynchronizationTimestamp(
            _config.LocalTransmitCaptureQuality,_config.LocalTransmitCaptureUncertainty);
        if(!ValidCapture(t1)) return false;
        std::uint32_t sequence=_nextSequence++;
        if(sequence==0) sequence=_nextSequence++;
        if(sequence==0) return false;
        if(!SocketClockWireV2::EncodeRequest(sequence,output,capacity)) return false;
        _pendingSequence=sequence;
        _pendingT1=t1;
        bytes=SocketClockWireV2::RequestBytes;
        return true;
    }

    bool ProcessRequest(
        const std::uint8_t* data,std::size_t size,
        const Timing::ClockTimestampCapture<>& t2,
        SocketClockResponseSink responseSink) noexcept {
        if(!IsReference() || !ValidCapture(t2) || !responseSink) return false;
        std::uint32_t sequence=0;
        if(!SocketClockWireV2::DecodeRequest(data,size,sequence)) return false;
        const auto t3=_target->CaptureSynchronizationTimestamp(
            _config.LocalTransmitCaptureQuality,_config.LocalTransmitCaptureUncertainty);
        if(!ValidCapture(t3) || t3.MonotonicTimeNanoseconds<t2.MonotonicTimeNanoseconds ||
           t3.SystemTimeNanoseconds<t2.SystemTimeNanoseconds) return false;
        const auto status=_target->GetSynchronizationStatus();
        SocketClockWireV2::Response response{};
        response.Common={SocketClockWireV2::Kind::Response,sequence};
        response.T2=t2;
        response.T3=t3;
        response.ReferenceReliability=status.Reliability;
        response.ReferenceUncertainty=status.CurrentUncertainty;
        std::array<std::uint8_t,SocketClockWireV2::ResponseBytes> wire{};
        if(!SocketClockWireV2::EncodeResponse(response,wire.data(),wire.size())) return false;
        return responseSink.Send(responseSink.Owner,wire.data(),wire.size());
    }

    bool ProcessResponse(
        const std::uint8_t* data,std::size_t size,
        const Timing::ClockTimestampCapture<>& t4) noexcept {
        if(!IsClient() || !ValidCapture(t4) || _pendingSequence==0 || !ValidCapture(_pendingT1)) return false;
        SocketClockWireV2::Response response{};
        if(!SocketClockWireV2::DecodeResponse(data,size,response) || response.Common.Sequence!=_pendingSequence) return false;
        const auto t1=_pendingT1;
        CancelPendingRequest();
        Timing::ClockSynchronizationObservation<> observation{};
        observation.T1=t1;
        observation.T2=response.T2;
        observation.T3=response.T3;
        observation.T4=t4;
        observation.ReferenceIdentity=_config.ReferenceIdentity;
        observation.ReferenceReliability=response.ReferenceReliability;
        observation.ReferenceUncertainty=response.ReferenceUncertainty;
        return _target->SubmitSynchronizationObservation(observation).Accepted;
    }

    void CancelPendingRequest() noexcept {
        _pendingSequence=0;
        _pendingT1={};
    }

    Timing::ClockSynchronizationStatus GetSynchronizationStatus() const noexcept {
        return _target==nullptr ? Timing::ClockSynchronizationStatus{} : _target->GetSynchronizationStatus();
    }
};

} // namespace ESPressio::Sockets
