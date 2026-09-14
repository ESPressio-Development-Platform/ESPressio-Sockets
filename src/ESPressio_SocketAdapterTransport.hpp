#pragma once

#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <limits>

#include <ESPressio_AdapterRuntime.hpp>
#include <ESPressio_AdapterTransport.hpp>
#include <ESPressio_AdapterProvenance.hpp>
#include <ESPressio_PrimitiveAdmission.hpp>
#include <ESPressio_PrimitivePolicy.hpp>

namespace ESPressio::Sockets {

/// <summary>Socket-neutral frame kinds used only between A2 socket bindings.</summary>
enum class SocketAdapterFrameKind : std::uint8_t { Primitive=1, AdmissionReceipt=2 };
/// <summary>Whether one frozen socket route is datagram- or stream-framed.</summary>
enum class SocketAdapterSessionMode : std::uint8_t { Datagram=0, Stream=1 };
/// <summary>Nonblocking result returned by one concrete socket/session writer.</summary>
enum class SocketAdapterWriteDisposition : std::uint8_t { Accepted=0, TemporarilyUnavailable=1, ResourceUnavailable=2, PermanentlyRejected=3 };
/// <summary>Bounded ingress parser result. Stream callers use Consumed to resubmit remaining bytes later.</summary>
enum class SocketAdapterFeedStatus : std::uint8_t { Accepted=0, Partial=1, Busy=2, ResourceUnavailable=3, Unsupported=4, Rejected=5, Malformed=6, NotRunning=7 };

struct SocketAdapterFeedResult final { SocketAdapterFeedStatus Status=SocketAdapterFeedStatus::Malformed; std::size_t Consumed=0; };

/// <summary>Fixed writer target. Accepted means the concrete session synchronously copied/owned the complete frame.</summary>
using SocketAdapterWriteThunk=SocketAdapterWriteDisposition(*)(void*,const std::uint8_t*,std::size_t) noexcept;
struct SocketAdapterWriteTarget final { void* Owner=nullptr; SocketAdapterWriteThunk Write=nullptr; constexpr explicit operator bool() const noexcept { return Owner&&Write; } };

/// <summary>Coalesced infrastructure wake used for receipt/completion service and writable-capacity changes.</summary>
using SocketAdapterWakeThunk=void(*)(void*) noexcept;
struct SocketAdapterWakeTarget final { void* Owner=nullptr; SocketAdapterWakeThunk Wake=nullptr; constexpr explicit operator bool() const noexcept { return Owner&&Wake; } void Signal() const noexcept { if(*this) Wake(Owner); } };

/// <summary>Family-neutral policy-resolution outcome for an already bounded socket Primitive frame.</summary>
enum class SocketAdapterPolicyResolutionStatus : std::uint8_t { Success=0, Unsupported=1, Rejected=2, Malformed=3 };
using SocketAdapterPolicyResolveThunk=SocketAdapterPolicyResolutionStatus(*)(void*,Primitive::PrimitiveFamilyId,Primitive::PrimitiveProtocolVersion,Adapters::AdapterServiceClass,Adapters::AdapterByteView,Primitive::PrimitivePolicyDescriptor&) noexcept;
struct SocketAdapterPolicyResolver final { void* Owner=nullptr; SocketAdapterPolicyResolveThunk Resolve=nullptr; constexpr explicit operator bool() const noexcept { return Owner&&Resolve; } };

namespace SocketAdapterWire {
inline constexpr std::uint32_t Magic=0x31413253u;
inline constexpr std::uint8_t Version=1;
inline constexpr std::uint8_t ReceiptRequiredFlag=0x01u;
inline constexpr std::size_t HeaderBytes=24;

inline void Write16(std::uint8_t* out,std::uint16_t value) noexcept { out[0]=static_cast<std::uint8_t>(value); out[1]=static_cast<std::uint8_t>(value>>8); }
inline void Write32(std::uint8_t* out,std::uint32_t value) noexcept { for(std::size_t i=0;i<4;++i) out[i]=static_cast<std::uint8_t>(value>>(8*i)); }
inline void Write64(std::uint8_t* out,std::uint64_t value) noexcept { for(std::size_t i=0;i<8;++i) out[i]=static_cast<std::uint8_t>(value>>(8*i)); }
inline std::uint16_t Read16(const std::uint8_t* in) noexcept { return static_cast<std::uint16_t>(in[0])|static_cast<std::uint16_t>(static_cast<std::uint16_t>(in[1])<<8); }
inline std::uint32_t Read32(const std::uint8_t* in) noexcept { std::uint32_t value=0; for(std::size_t i=0;i<4;++i) value|=static_cast<std::uint32_t>(in[i])<<(8*i); return value; }
inline std::uint64_t Read64(const std::uint8_t* in) noexcept { std::uint64_t value=0; for(std::size_t i=0;i<8;++i) value|=static_cast<std::uint64_t>(in[i])<<(8*i); return value; }

struct Header final {
    SocketAdapterFrameKind Kind=SocketAdapterFrameKind::Primitive;
    Adapters::AdapterServiceClass Service=Adapters::AdapterServiceClass::BestEffort;
    std::uint8_t Flags=0;
    Primitive::PrimitiveFamilyId Family=0;
    Primitive::PrimitiveProtocolVersion Protocol=0;
    std::uint64_t Correlation=0;
    std::uint32_t PayloadBytes=0;
};

inline bool EncodeHeader(const Header& header,std::uint8_t* out,std::size_t capacity) noexcept {
    if(!out||capacity<HeaderBytes) return false;
    Write32(out,Magic); out[4]=Version; out[5]=static_cast<std::uint8_t>(header.Kind);
    out[6]=static_cast<std::uint8_t>(header.Service); out[7]=header.Flags;
    Write16(out+8,header.Family); Write16(out+10,header.Protocol);
    Write64(out+12,header.Correlation); Write32(out+20,header.PayloadBytes); return true;
}
inline bool DecodeHeader(const std::uint8_t* in,std::size_t size,Header& header) noexcept {
    if(!in||size<HeaderBytes||Read32(in)!=Magic||in[4]!=Version) return false;
    if(in[5]!=static_cast<std::uint8_t>(SocketAdapterFrameKind::Primitive)&&in[5]!=static_cast<std::uint8_t>(SocketAdapterFrameKind::AdmissionReceipt)) return false;
    if(in[6]>=Adapters::AdapterServiceClassCount||(in[7]&~ReceiptRequiredFlag)!=0) return false;
    header.Kind=static_cast<SocketAdapterFrameKind>(in[5]); header.Service=static_cast<Adapters::AdapterServiceClass>(in[6]); header.Flags=in[7];
    header.Family=Read16(in+8); header.Protocol=Read16(in+10); header.Correlation=Read64(in+12); header.PayloadBytes=Read32(in+20);
    if(header.Family==0||header.Protocol==0) return false;
    if(header.Kind==SocketAdapterFrameKind::AdmissionReceipt&&(header.Flags!=0||header.Correlation==0||header.PayloadBytes!=1)) return false;
    if(header.Kind==SocketAdapterFrameKind::Primitive&&((header.Flags&ReceiptRequiredFlag)!=0)&&header.Correlation==0) return false;
    return true;
}
} // namespace SocketAdapterWire

/// <summary>Fixed-capacity, family-neutral socket/session lower transport for A2.</summary>
/// <remarks>Owns socket framing, route/session lifecycle, finite stream assembly and exact M1 receipt transport only. It never parses Event/Command/State representations and never upgrades a local write into destination admission.</remarks>
template<class TAdapterRuntime,std::size_t TMaximumSessions,std::size_t TMaximumPendingOutbound,std::size_t TMaximumPendingInboundReceipts,std::size_t TMaximumFrameBytes>
class SocketAdapterTransport final {
    static_assert(TMaximumSessions>0,"Socket Adapter requires at least one session slot");
    static_assert(TMaximumPendingOutbound>0,"Socket Adapter requires outbound correlation capacity");
    static_assert(TMaximumPendingInboundReceipts>0,"Socket Adapter requires inbound receipt capacity");
    static_assert(TMaximumFrameBytes>SocketAdapterWire::HeaderBytes+1,"Socket Adapter frame capacity is too small");

    struct Session final {
        bool Used=false; bool Available=false; Adapters::AdapterRouteToken Route{}; SocketAdapterSessionMode Mode=SocketAdapterSessionMode::Datagram;
        SocketAdapterWriteTarget Writer{}; Adapters::AdapterSemanticProvenance Provenance{}; std::uint64_t Generation=1;
        std::array<std::uint8_t,TMaximumFrameBytes> Stream{}; std::size_t StreamBytes=0; std::size_t ExpectedBytes=0; std::atomic_flag RxBusy=ATOMIC_FLAG_INIT;
    };
    struct OutboundPending final {
        bool Used=false; bool Ready=false; std::uint64_t Correlation=0; Adapters::AdapterRecordIdentity Record{}; Adapters::AdapterRouteToken Route{};
        std::uint64_t TransportGeneration=0; std::uint64_t SessionGeneration=0; Primitive::PrimitiveFamilyId Family=0; Primitive::PrimitiveProtocolVersion Protocol=0;
        Adapters::AdapterServiceClass Service=Adapters::AdapterServiceClass::BestEffort; Adapters::LowerTransportDisposition Disposition=Adapters::LowerTransportDisposition::TemporarilyUnavailable;
        Primitive::PrimitiveAdmissionDisposition Admission=Primitive::PrimitiveAdmissionDisposition::Rejected; bool HasAdmission=false;
    };
    struct InboundReceipt final {
        bool Used=false; bool Ready=false; std::uint64_t LocalCorrelation=0; std::uint64_t RemoteCorrelation=0; Adapters::AdapterRouteToken Route{};
        std::uint64_t TransportGeneration=0; std::uint64_t SessionGeneration=0; Primitive::PrimitiveFamilyId Family=0; Primitive::PrimitiveProtocolVersion Protocol=0;
        Adapters::AdapterServiceClass Service=Adapters::AdapterServiceClass::BestEffort; Primitive::PrimitiveAdmissionDisposition Admission=Primitive::PrimitiveAdmissionDisposition::Rejected;
    };

    TAdapterRuntime* _adapter=nullptr;
    SocketAdapterPolicyResolver _policyResolver{};
    SocketAdapterWakeTarget _wake{};
    std::array<Session,TMaximumSessions> _sessions{};
    std::array<OutboundPending,TMaximumPendingOutbound> _outbound{};
    std::array<InboundReceipt,TMaximumPendingInboundReceipts> _inboundReceipts{};
    std::array<std::uint8_t,TMaximumFrameBytes> _tx{};
    std::atomic_flag _txBusy=ATOMIC_FLAG_INIT;
    std::uint64_t _nextWireCorrelation=1;
    std::uint64_t _nextInboundCorrelation=1;
    std::uint64_t _generation=1;
    std::uint8_t _serviceMask=0x3Fu;
    bool _frozen=false;
    std::atomic<bool> _active{false};

    struct FlagGuard final { std::atomic_flag* Flag=nullptr; explicit FlagGuard(std::atomic_flag& flag) noexcept:Flag(&flag) {} ~FlagGuard(){ if(Flag) Flag->clear(std::memory_order_release); } FlagGuard(const FlagGuard&)=delete; FlagGuard& operator=(const FlagGuard&)=delete; };

    Session* FindSession(Adapters::AdapterRouteToken route) noexcept { if(!route) return nullptr; for(auto& session:_sessions) if(session.Used&&session.Route.Value==route.Value) return &session; return nullptr; }
    const Session* FindSession(Adapters::AdapterRouteToken route) const noexcept { if(!route) return nullptr; for(const auto& session:_sessions) if(session.Used&&session.Route.Value==route.Value) return &session; return nullptr; }
    static Adapters::LowerTransportDisposition MapWrite(SocketAdapterWriteDisposition value) noexcept {
        switch(value){ case SocketAdapterWriteDisposition::Accepted:return Adapters::LowerTransportDisposition::Accepted; case SocketAdapterWriteDisposition::TemporarilyUnavailable:return Adapters::LowerTransportDisposition::TemporarilyUnavailable; case SocketAdapterWriteDisposition::ResourceUnavailable:return Adapters::LowerTransportDisposition::ResourceUnavailable; case SocketAdapterWriteDisposition::PermanentlyRejected:return Adapters::LowerTransportDisposition::PermanentlyRejected; }
        return Adapters::LowerTransportDisposition::PermanentlyRejected;
    }
    static Primitive::PrimitiveAdmissionDisposition MapImmediateIngress(Adapters::AdapterSubmissionDisposition value) noexcept {
        switch(value){
            case Adapters::AdapterSubmissionDisposition::Accepted:return Primitive::PrimitiveAdmissionDisposition::Accepted;
            case Adapters::AdapterSubmissionDisposition::Busy:return Primitive::PrimitiveAdmissionDisposition::TemporarilyUnavailable;
            case Adapters::AdapterSubmissionDisposition::ResourceUnavailable:return Primitive::PrimitiveAdmissionDisposition::ResourceUnavailable;
            case Adapters::AdapterSubmissionDisposition::Unsupported:return Primitive::PrimitiveAdmissionDisposition::Unsupported;
            case Adapters::AdapterSubmissionDisposition::Rejected:return Primitive::PrimitiveAdmissionDisposition::Rejected;
            case Adapters::AdapterSubmissionDisposition::Malformed:return Primitive::PrimitiveAdmissionDisposition::Malformed;
            case Adapters::AdapterSubmissionDisposition::RepresentationTooLarge:
            case Adapters::AdapterSubmissionDisposition::InvalidConfiguration:return Primitive::PrimitiveAdmissionDisposition::Malformed;
            case Adapters::AdapterSubmissionDisposition::NotRunning:return Primitive::PrimitiveAdmissionDisposition::TemporarilyUnavailable;
        }
        return Primitive::PrimitiveAdmissionDisposition::Rejected;
    }
    static Primitive::PrimitiveAdmissionDisposition MapResolve(SocketAdapterPolicyResolutionStatus value) noexcept {
        switch(value){ case SocketAdapterPolicyResolutionStatus::Unsupported:return Primitive::PrimitiveAdmissionDisposition::Unsupported; case SocketAdapterPolicyResolutionStatus::Rejected:return Primitive::PrimitiveAdmissionDisposition::Rejected; case SocketAdapterPolicyResolutionStatus::Malformed:return Primitive::PrimitiveAdmissionDisposition::Malformed; case SocketAdapterPolicyResolutionStatus::Success:return Primitive::PrimitiveAdmissionDisposition::Accepted; }
        return Primitive::PrimitiveAdmissionDisposition::Rejected;
    }
    std::uint64_t AllocateWireCorrelation() noexcept { if(_nextWireCorrelation==0||_nextWireCorrelation==std::numeric_limits<std::uint64_t>::max()) return 0; return _nextWireCorrelation++; }
    std::uint64_t AllocateInboundCorrelation() noexcept { if(_nextInboundCorrelation==0||_nextInboundCorrelation==std::numeric_limits<std::uint64_t>::max()) return 0; return _nextInboundCorrelation++; }
    OutboundPending* ReserveOutbound() noexcept { for(auto& pending:_outbound) if(!pending.Used){ pending=OutboundPending{}; pending.Used=true; return &pending; } return nullptr; }
    InboundReceipt* ReserveInboundReceipt() noexcept { for(auto& receipt:_inboundReceipts) if(!receipt.Used){ receipt=InboundReceipt{}; receipt.Used=true; return &receipt; } return nullptr; }
    void ResetSessionStream(Session& session) noexcept { session.StreamBytes=0; session.ExpectedBytes=0; }
    bool AdvanceSessionGeneration(Session& session) noexcept { if(session.Generation==std::numeric_limits<std::uint64_t>::max()) return false; ++session.Generation; return true; }
    bool BuildFrame(const SocketAdapterWire::Header& header,const std::uint8_t* payload,std::size_t payloadBytes,std::size_t& frameBytes) noexcept {
        if(payloadBytes>TMaximumFrameBytes-SocketAdapterWire::HeaderBytes||payloadBytes>std::numeric_limits<std::uint32_t>::max()) return false;
        auto actual=header; actual.PayloadBytes=static_cast<std::uint32_t>(payloadBytes); if(!SocketAdapterWire::EncodeHeader(actual,_tx.data(),_tx.size())) return false;
        if(payloadBytes) {
            std::memcpy(_tx.data()+SocketAdapterWire::HeaderBytes,payload,payloadBytes);
        }
        frameBytes=SocketAdapterWire::HeaderBytes+payloadBytes;
        return true;
    }
    void MarkSessionPendingUnavailable(Session& session) noexcept {
        bool wake=false;
        for(auto& pending:_outbound) if(pending.Used&&!pending.Ready&&pending.Route.Value==session.Route.Value&&pending.SessionGeneration==session.Generation){ pending.Ready=true; pending.HasAdmission=false; pending.Disposition=Adapters::LowerTransportDisposition::TemporarilyUnavailable; wake=true; }
        for(auto& receipt:_inboundReceipts) if(receipt.Used&&receipt.Route.Value==session.Route.Value) receipt=InboundReceipt{};
        if(wake) _wake.Signal();
    }
    void OnInboundCompletion(const Adapters::AdapterInboundCompletion& completion) noexcept {
        if(!_active.load(std::memory_order_acquire)) return;
        for(auto& receipt:_inboundReceipts){
            if(!receipt.Used||receipt.LocalCorrelation!=completion.Correlation) continue;
            const auto* session=FindSession(receipt.Route);
            if(!session||!session->Available||receipt.TransportGeneration!=_generation||receipt.SessionGeneration!=session->Generation){ receipt=InboundReceipt{}; return; }
            receipt.Admission=completion.Admission; receipt.Ready=true; _wake.Signal(); return;
        }
    }
    static void CompleteInboundThunk(void* owner,const Adapters::AdapterInboundCompletion& completion) noexcept { static_cast<SocketAdapterTransport*>(owner)->OnInboundCompletion(completion); }

    SocketAdapterFeedStatus ProcessPrimitive(Session& session,const SocketAdapterWire::Header& header,const std::uint8_t* payload,std::size_t payloadBytes) noexcept {
        if(!_adapter||!_policyResolver) return SocketAdapterFeedStatus::NotRunning;
        Primitive::PrimitivePolicyDescriptor policy{};
        const auto resolved=_policyResolver.Resolve(_policyResolver.Owner,header.Family,header.Protocol,header.Service,{payload,payloadBytes},policy);
        const bool requested=(header.Flags&SocketAdapterWire::ReceiptRequiredFlag)!=0;
        if(resolved!=SocketAdapterPolicyResolutionStatus::Success){
            if(requested){
                auto* receipt=ReserveInboundReceipt(); if(!receipt) return SocketAdapterFeedStatus::ResourceUnavailable;
                receipt->LocalCorrelation=AllocateInboundCorrelation(); if(!receipt->LocalCorrelation){ *receipt=InboundReceipt{}; return SocketAdapterFeedStatus::ResourceUnavailable; }
                receipt->RemoteCorrelation=header.Correlation; receipt->Route=session.Route; receipt->TransportGeneration=_generation; receipt->SessionGeneration=session.Generation;
                receipt->Family=header.Family; receipt->Protocol=header.Protocol; receipt->Service=header.Service; receipt->Admission=MapResolve(resolved); receipt->Ready=true; _wake.Signal();
            }
            return resolved==SocketAdapterPolicyResolutionStatus::Unsupported?SocketAdapterFeedStatus::Unsupported:resolved==SocketAdapterPolicyResolutionStatus::Rejected?SocketAdapterFeedStatus::Rejected:SocketAdapterFeedStatus::Malformed;
        }
        const bool policyRequiresReceipt=policy.Evidence!=0; if(requested!=policyRequiresReceipt) return SocketAdapterFeedStatus::Rejected;
        InboundReceipt* receipt=nullptr; Adapters::AdapterInboundCompletionTarget completion{};
        if(requested){
            receipt=ReserveInboundReceipt(); if(!receipt) return SocketAdapterFeedStatus::ResourceUnavailable;
            const auto local=AllocateInboundCorrelation(); if(!local){ *receipt=InboundReceipt{}; return SocketAdapterFeedStatus::ResourceUnavailable; }
            receipt->LocalCorrelation=local; receipt->RemoteCorrelation=header.Correlation; receipt->Route=session.Route; receipt->TransportGeneration=_generation; receipt->SessionGeneration=session.Generation;
            receipt->Family=header.Family; receipt->Protocol=header.Protocol; receipt->Service=header.Service; completion={this,&SocketAdapterTransport::CompleteInboundThunk};
        }
        const auto admitted=_adapter->AdmitTrustedInbound(header.Family,header.Service,header.Protocol,{payload,payloadBytes},session.Provenance,session.Route,policy,receipt?receipt->LocalCorrelation:0,completion);
        if(admitted==Adapters::AdapterSubmissionDisposition::Accepted) return SocketAdapterFeedStatus::Accepted;
        if(receipt){ receipt->Admission=MapImmediateIngress(admitted); receipt->Ready=true; _wake.Signal(); }
        switch(admitted){
            case Adapters::AdapterSubmissionDisposition::Busy:return SocketAdapterFeedStatus::Busy;
            case Adapters::AdapterSubmissionDisposition::ResourceUnavailable:return SocketAdapterFeedStatus::ResourceUnavailable;
            case Adapters::AdapterSubmissionDisposition::Unsupported:return SocketAdapterFeedStatus::Unsupported;
            case Adapters::AdapterSubmissionDisposition::Rejected:return SocketAdapterFeedStatus::Rejected;
            case Adapters::AdapterSubmissionDisposition::Malformed:
            case Adapters::AdapterSubmissionDisposition::RepresentationTooLarge:
            case Adapters::AdapterSubmissionDisposition::InvalidConfiguration:return SocketAdapterFeedStatus::Malformed;
            case Adapters::AdapterSubmissionDisposition::NotRunning:return SocketAdapterFeedStatus::NotRunning;
            case Adapters::AdapterSubmissionDisposition::Accepted:return SocketAdapterFeedStatus::Accepted;
        }
        return SocketAdapterFeedStatus::Rejected;
    }
    SocketAdapterFeedStatus ProcessReceipt(Session& session,const SocketAdapterWire::Header& header,const std::uint8_t* payload,std::size_t payloadBytes) noexcept {
        if(payloadBytes!=1||payload[0]>static_cast<std::uint8_t>(Primitive::PrimitiveAdmissionDisposition::Malformed)) return SocketAdapterFeedStatus::Malformed;
        for(auto& pending:_outbound){
            if(!pending.Used||pending.Ready||pending.Correlation!=header.Correlation||pending.Route.Value!=session.Route.Value||pending.TransportGeneration!=_generation||pending.SessionGeneration!=session.Generation) continue;
            if(pending.Family!=header.Family||pending.Protocol!=header.Protocol||pending.Service!=header.Service) return SocketAdapterFeedStatus::Rejected;
            pending.Admission=static_cast<Primitive::PrimitiveAdmissionDisposition>(payload[0]); pending.HasAdmission=true; pending.Disposition=Adapters::LowerTransportDisposition::Accepted; pending.Ready=true; _wake.Signal(); return SocketAdapterFeedStatus::Accepted;
        }
        return SocketAdapterFeedStatus::Rejected;
    }
    SocketAdapterFeedStatus ProcessFrame(Session& session,const std::uint8_t* frame,std::size_t size) noexcept {
        SocketAdapterWire::Header header{}; if(!SocketAdapterWire::DecodeHeader(frame,size,header)) return SocketAdapterFeedStatus::Malformed;
        const std::size_t total=SocketAdapterWire::HeaderBytes+static_cast<std::size_t>(header.PayloadBytes); if(total!=size||total>TMaximumFrameBytes) return SocketAdapterFeedStatus::Malformed;
        const auto* payload=frame+SocketAdapterWire::HeaderBytes; return header.Kind==SocketAdapterFrameKind::Primitive?ProcessPrimitive(session,header,payload,header.PayloadBytes):ProcessReceipt(session,header,payload,header.PayloadBytes);
    }
    static Adapters::LowerTransportSubmitResult SubmitThunk(void* owner,Adapters::AdapterRecordIdentity record,Primitive::PrimitiveFamilyId family,Primitive::PrimitiveProtocolVersion protocol,const Primitive::PrimitivePolicyDescriptor& policy,Adapters::AdapterServiceClass service,Adapters::AdapterByteView bytes,Adapters::AdapterRouteToken route) noexcept { return static_cast<SocketAdapterTransport*>(owner)->Submit(record,family,protocol,policy,service,bytes,route); }
    static bool ValidateThunk(void* owner) noexcept { return static_cast<SocketAdapterTransport*>(owner)->Validate(); }
    static void CancelThunk(void* owner,Adapters::AdapterRecordIdentity record) noexcept { static_cast<SocketAdapterTransport*>(owner)->Cancel(record); }
    static void QuiesceThunk(void* owner) noexcept { static_cast<SocketAdapterTransport*>(owner)->Quiesce(); }

public:
    /// <summary>Creates an unstarted neutral transport bound to one A2 runtime and frozen policy resolver.</summary>
    SocketAdapterTransport(TAdapterRuntime& adapter,SocketAdapterPolicyResolver resolver,SocketAdapterWakeTarget wake={}) noexcept:_adapter(&adapter),_policyResolver(resolver),_wake(wake) {}
    SocketAdapterTransport(const SocketAdapterTransport&)=delete; SocketAdapterTransport& operator=(const SocketAdapterTransport&)=delete;

    /// <summary>Adds one immutable route/session topology entry before Freeze.</summary>
    bool BindSession(Adapters::AdapterRouteToken route,SocketAdapterSessionMode mode,SocketAdapterWriteTarget writer,const Adapters::AdapterSemanticProvenance& provenance,bool initiallyAvailable=false) noexcept {
        if(_frozen||!route||!writer||FindSession(route)) return false;
        for(auto& session:_sessions) if(!session.Used){ session.Used=true; session.Available=initiallyAvailable; session.Route=route; session.Mode=mode; session.Writer=writer; session.Provenance=provenance; session.Generation=1; ResetSessionStream(session); return true; }
        return false;
    }
    /// <summary>Freezes route topology and neutral service support.</summary>
    bool Freeze(std::uint8_t serviceMask=0x3Fu) noexcept {
        if(_frozen||!_policyResolver||serviceMask==0||(serviceMask&~0x3Fu)!=0) return false;
        bool any=false; for(const auto& session:_sessions) any=any||session.Used; if(!any) return false;
        _serviceMask=serviceMask; _frozen=true; return true;
    }
    /// <summary>Publishes the initial active lifecycle after A2 is ready to accept ingress.</summary>
    bool Start() noexcept { if(!_frozen||!_adapter||_active.exchange(true,std::memory_order_acq_rel)) return _active.load(); return true; }
    /// <summary>Starts a replacement lifecycle against a replacement A2 runtime and invalidates all old volatile correlation.</summary>
    bool Restart(TAdapterRuntime& replacement) noexcept {
        if(!_frozen||_active.load(std::memory_order_acquire)||_generation==std::numeric_limits<std::uint64_t>::max()) return false;
        ++_generation; _adapter=&replacement; for(auto& pending:_outbound) pending=OutboundPending{}; for(auto& receipt:_inboundReceipts) receipt=InboundReceipt{};
        for(auto& session:_sessions) if(session.Used){ ResetSessionStream(session); if(!AdvanceSessionGeneration(session)) session.Available=false; }
        _active.store(true,std::memory_order_release); return true;
    }
    /// <summary>Rejects new work and releases all transport-owned volatile correlation/buffering without draining application work.</summary>
    void Quiesce() noexcept { if(!_active.exchange(false,std::memory_order_acq_rel)) return; for(auto& pending:_outbound) pending=OutboundPending{}; for(auto& receipt:_inboundReceipts) receipt=InboundReceipt{}; for(auto& session:_sessions) if(session.Used) ResetSessionStream(session); }
    bool Validate() const noexcept { return _frozen&&static_cast<bool>(_policyResolver); }
    bool IsActive() const noexcept { return _active.load(std::memory_order_acquire); }
    std::uint64_t Generation() const noexcept { return _generation; }

    /// <summary>Publishes a connection/session availability transition and invalidates the old session generation.</summary>
    bool SetSessionAvailable(Adapters::AdapterRouteToken route,bool available) noexcept {
        auto* session=FindSession(route); if(!_frozen||!session) return false; if(session->Available==available) return true;
        if(!available) {
            MarkSessionPendingUnavailable(*session);
        }
        if(!AdvanceSessionGeneration(*session)){ session->Available=false; return false; }
        session->Available=available; ResetSessionStream(*session); _wake.Signal(); return true;
    }
    /// <summary>Wakes bounded service after an external socket implementation reports writable capacity.</summary>
    void NotifyWritable(Adapters::AdapterRouteToken route) noexcept { const auto* session=FindSession(route); if(session&&session->Available) _wake.Signal(); }

    /// <summary>Returns the frozen generic A2 lower-transport binding.</summary>
    Adapters::LowerTransportBinding Binding() noexcept {
        bool allValidated=true; bool any=false; for(const auto& session:_sessions) if(session.Used){ any=true; allValidated=allValidated&&static_cast<bool>(session.Provenance.OriginalSource); }
        Adapters::LowerTransportBinding binding{}; if(!_frozen||!any) return binding;
        binding.Owner=this; binding.Submit=&SocketAdapterTransport::SubmitThunk; binding.Validate=&SocketAdapterTransport::ValidateThunk; binding.Cancel=&SocketAdapterTransport::CancelThunk; binding.Quiesce=&SocketAdapterTransport::QuiesceThunk;
        binding.ServiceClassMask=_serviceMask; binding.ProvidesDestinationPrimitiveAdmission=true; binding.ProvidesValidatedOriginalSource=allValidated; return binding;
    }

    /// <summary>Nonblockingly frames and submits one immutable A2 payload to its frozen socket route.</summary>
    Adapters::LowerTransportSubmitResult Submit(Adapters::AdapterRecordIdentity record,Primitive::PrimitiveFamilyId family,Primitive::PrimitiveProtocolVersion protocol,const Primitive::PrimitivePolicyDescriptor& policy,Adapters::AdapterServiceClass service,Adapters::AdapterByteView bytes,Adapters::AdapterRouteToken route) noexcept {
        if(!_active.load(std::memory_order_acquire)) return {Adapters::LowerTransportDisposition::TemporarilyUnavailable,_generation,false};
        auto* session=FindSession(route); if(!session||!session->Available||!session->Writer) return {Adapters::LowerTransportDisposition::TemporarilyUnavailable,_generation,false};
        if(static_cast<std::uint8_t>(service)>=Adapters::AdapterServiceClassCount||!(_serviceMask&(std::uint8_t{1}<<static_cast<std::uint8_t>(service)))) return {Adapters::LowerTransportDisposition::PermanentlyRejected,_generation,false};
        if(bytes.Size&&!bytes.Data) return {Adapters::LowerTransportDisposition::PermanentlyRejected,_generation,false};
        if(bytes.Size>TMaximumFrameBytes-SocketAdapterWire::HeaderBytes) return {Adapters::LowerTransportDisposition::PermanentlyRejected,_generation,false};
        if(_txBusy.test_and_set(std::memory_order_acquire)) {
            return {Adapters::LowerTransportDisposition::TemporarilyUnavailable,_generation,false};
        }
        FlagGuard tx(_txBusy);
        OutboundPending* pending=nullptr; std::uint64_t correlation=0; const bool requireReceipt=policy.Evidence!=0;
        if(requireReceipt){ pending=ReserveOutbound(); if(!pending) return {Adapters::LowerTransportDisposition::ResourceUnavailable,_generation,false}; correlation=AllocateWireCorrelation(); if(!correlation){ *pending=OutboundPending{}; return {Adapters::LowerTransportDisposition::ResourceUnavailable,_generation,false}; }
            pending->Correlation=correlation; pending->Record=record; pending->Route=route; pending->TransportGeneration=_generation; pending->SessionGeneration=session->Generation; pending->Family=family; pending->Protocol=protocol; pending->Service=service; }
        SocketAdapterWire::Header header{}; header.Kind=SocketAdapterFrameKind::Primitive; header.Service=service; header.Flags=requireReceipt?SocketAdapterWire::ReceiptRequiredFlag:0; header.Family=family; header.Protocol=protocol; header.Correlation=correlation;
        std::size_t frameBytes=0; if(!BuildFrame(header,bytes.Data,bytes.Size,frameBytes)){ if(pending) *pending=OutboundPending{}; return {Adapters::LowerTransportDisposition::PermanentlyRejected,_generation,false}; }
        const auto wrote=session->Writer.Write(session->Writer.Owner,_tx.data(),frameBytes); if(wrote!=SocketAdapterWriteDisposition::Accepted){ if(pending) *pending=OutboundPending{}; return {MapWrite(wrote),_generation,false}; }
        return {Adapters::LowerTransportDisposition::Accepted,_generation,requireReceipt};
    }

    /// <summary>Cancels transport-owned correlation for one exact A2 record.</summary>
    void Cancel(Adapters::AdapterRecordIdentity record) noexcept { for(auto& pending:_outbound) if(pending.Used&&pending.Record==record) pending=OutboundPending{}; }

    /// <summary>Admits one complete datagram frame through bounded envelope validation into A2.</summary>
    SocketAdapterFeedResult FeedDatagram(Adapters::AdapterRouteToken route,const std::uint8_t* bytes,std::size_t size) noexcept {
        if(!_active.load(std::memory_order_acquire)) {
            return {SocketAdapterFeedStatus::NotRunning,0};
        }
        auto* session=FindSession(route);
        if(!session||!session->Available||session->Mode!=SocketAdapterSessionMode::Datagram) return {SocketAdapterFeedStatus::Rejected,0};
        if(!bytes||size<SocketAdapterWire::HeaderBytes||size>TMaximumFrameBytes) return {SocketAdapterFeedStatus::Malformed,size};
        if(session->RxBusy.test_and_set(std::memory_order_acquire)) {
            return {SocketAdapterFeedStatus::Busy,0};
        }
        FlagGuard guard(session->RxBusy);
        return {ProcessFrame(*session,bytes,size),size};
    }

    /// <summary>Consumes at most one complete stream frame per bounded ingress quantum.</summary>
    SocketAdapterFeedResult FeedStream(Adapters::AdapterRouteToken route,const std::uint8_t* bytes,std::size_t size) noexcept {
        if(!_active.load(std::memory_order_acquire)) {
            return {SocketAdapterFeedStatus::NotRunning,0};
        }
        auto* session=FindSession(route);
        if(!session||!session->Available||session->Mode!=SocketAdapterSessionMode::Stream) return {SocketAdapterFeedStatus::Rejected,0};
        if((!bytes&&size)||session->RxBusy.test_and_set(std::memory_order_acquire)) {
            return {SocketAdapterFeedStatus::Busy,0};
        }
        FlagGuard guard(session->RxBusy);
        std::size_t consumed=0;
        while(consumed<size){
            const std::size_t target=session->ExpectedBytes?session->ExpectedBytes:SocketAdapterWire::HeaderBytes; const std::size_t need=target-session->StreamBytes; const std::size_t take=(size-consumed)<need?(size-consumed):need;
            if(take){ std::memcpy(session->Stream.data()+session->StreamBytes,bytes+consumed,take); session->StreamBytes+=take; consumed+=take; }
            if(session->StreamBytes<target) return {SocketAdapterFeedStatus::Partial,consumed};
            if(session->ExpectedBytes==0){ SocketAdapterWire::Header header{}; if(!SocketAdapterWire::DecodeHeader(session->Stream.data(),session->StreamBytes,header)){ ResetSessionStream(*session); return {SocketAdapterFeedStatus::Malformed,consumed}; }
                const std::size_t total=SocketAdapterWire::HeaderBytes+static_cast<std::size_t>(header.PayloadBytes); if(total>TMaximumFrameBytes){ ResetSessionStream(*session); return {SocketAdapterFeedStatus::Malformed,consumed}; } session->ExpectedBytes=total; if(session->StreamBytes<total) continue; }
            if(session->StreamBytes==session->ExpectedBytes){ const auto status=ProcessFrame(*session,session->Stream.data(),session->StreamBytes); ResetSessionStream(*session); return {status,consumed}; }
        }
        return {SocketAdapterFeedStatus::Partial,consumed};
    }

    /// <summary>Runs at most one deferred receipt/completion quantum. Sockets owns no polling worker for this service.</summary>
    bool ServiceOne() noexcept {
        if(!_active.load(std::memory_order_acquire)) return false;
        for(auto& receipt:_inboundReceipts){
            if(!receipt.Used||!receipt.Ready) continue;
            auto* session=FindSession(receipt.Route);
            if(!session||!session->Available||receipt.TransportGeneration!=_generation||receipt.SessionGeneration!=session->Generation){ receipt=InboundReceipt{}; return true; }
            if(_txBusy.test_and_set(std::memory_order_acquire)) return false;
            FlagGuard tx(_txBusy);
            SocketAdapterWire::Header header{}; header.Kind=SocketAdapterFrameKind::AdmissionReceipt; header.Service=receipt.Service; header.Family=receipt.Family; header.Protocol=receipt.Protocol; header.Correlation=receipt.RemoteCorrelation;
            const std::uint8_t payload=static_cast<std::uint8_t>(receipt.Admission); std::size_t frameBytes=0; if(!BuildFrame(header,&payload,1,frameBytes)){ receipt=InboundReceipt{}; return true; }
            const auto wrote=session->Writer.Write(session->Writer.Owner,_tx.data(),frameBytes); if(wrote==SocketAdapterWriteDisposition::Accepted||wrote==SocketAdapterWriteDisposition::PermanentlyRejected){ receipt=InboundReceipt{}; return true; } return false;
        }
        for(auto& pending:_outbound){
            if(!pending.Used||!pending.Ready) continue;
            if(pending.TransportGeneration!=_generation){ pending=OutboundPending{}; return true; }
            Adapters::LowerTransportCompletion completion{}; completion.Record=pending.Record; completion.TransportGeneration=pending.TransportGeneration; completion.Disposition=pending.Disposition; completion.DestinationAdmission=pending.Admission; completion.HasDestinationAdmission=pending.HasAdmission;
            const auto result=_adapter?_adapter->CompleteTransport(completion):Adapters::AdapterSubmissionDisposition::NotRunning; if(result==Adapters::AdapterSubmissionDisposition::Busy) return false; pending=OutboundPending{}; return true;
        }
        return false;
    }
};

} // namespace ESPressio::Sockets
