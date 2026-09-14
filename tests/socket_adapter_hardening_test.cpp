#include <ESPressio_SocketAdapterTransport.hpp>

#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <vector>

using namespace ESPressio;

namespace {

struct FakeAdapter final {
    Adapters::AdapterInboundCompletionTarget InboundCompletion{};
    std::uint64_t InboundCorrelation=0;
    std::size_t AdmitCount=0;
    std::size_t CompleteCount=0;
    Adapters::LowerTransportCompletion LastCompletion{};
    Adapters::AdapterSubmissionDisposition NextAdmission=Adapters::AdapterSubmissionDisposition::Accepted;

    Adapters::AdapterSubmissionDisposition AdmitTrustedInbound(
        Primitive::PrimitiveFamilyId,
        Adapters::AdapterServiceClass,
        Primitive::PrimitiveProtocolVersion,
        Adapters::AdapterByteView,
        const Adapters::AdapterSemanticProvenance&,
        Adapters::AdapterRouteToken,
        Primitive::PrimitivePolicyDescriptor,
        std::uint64_t correlation,
        Adapters::AdapterInboundCompletionTarget completion) noexcept {
        ++AdmitCount;
        InboundCorrelation=correlation;
        InboundCompletion=completion;
        return NextAdmission;
    }

    Adapters::AdapterSubmissionDisposition CompleteTransport(
        const Adapters::LowerTransportCompletion& completion) noexcept {
        ++CompleteCount;
        LastCompletion=completion;
        return Adapters::AdapterSubmissionDisposition::Accepted;
    }
};

struct Wire final {
    std::vector<std::uint8_t> Bytes;
    Sockets::SocketAdapterWriteDisposition Next=Sockets::SocketAdapterWriteDisposition::Accepted;

    static Sockets::SocketAdapterWriteDisposition Write(
        void* owner,const std::uint8_t* data,std::size_t size) noexcept {
        auto& self=*static_cast<Wire*>(owner);
        if(self.Next==Sockets::SocketAdapterWriteDisposition::Accepted)
            self.Bytes.assign(data,data+size);
        return self.Next;
    }
};

static Sockets::SocketAdapterPolicyResolutionStatus ResolvePolicy(
    void*,Primitive::PrimitiveFamilyId family,Primitive::PrimitiveProtocolVersion protocol,
    Adapters::AdapterServiceClass,Adapters::AdapterByteView bytes,
    Primitive::PrimitivePolicyDescriptor& policy) noexcept {
    if(family!=1||protocol!=1||!bytes.Data||bytes.Size==0)
        return Sockets::SocketAdapterPolicyResolutionStatus::Unsupported;
    policy={};
    policy.Category=1;
    policy.Evidence=bytes.Data[0]==0?0:1;
    policy.MaximumAttempts=1;
    return Sockets::SocketAdapterPolicyResolutionStatus::Success;
}

static void Wake(void* owner) noexcept { ++*static_cast<std::size_t*>(owner); }

static Adapters::AdapterRecordIdentity Record(std::uint64_t generation) noexcept {
    Adapters::AdapterRecordIdentity result{};
    result.Direction=Adapters::AdapterDirection::Outbound;
    result.Generation=generation;
    return result;
}

using Transport=Sockets::SocketAdapterTransport<FakeAdapter,1,1,1,128>;

std::vector<std::uint8_t> BuildReceipt(
    const std::vector<std::uint8_t>& primitive,
    Primitive::PrimitiveAdmissionDisposition admission) {
    Sockets::SocketAdapterWire::Header source{};
    assert(Sockets::SocketAdapterWire::DecodeHeader(primitive.data(),primitive.size(),source));
    std::vector<std::uint8_t> receipt(Sockets::SocketAdapterWire::HeaderBytes+1,0);
    Sockets::SocketAdapterWire::Header header{};
    header.Kind=Sockets::SocketAdapterFrameKind::AdmissionReceipt;
    header.Service=source.Service;
    header.Family=source.Family;
    header.Protocol=source.Protocol;
    header.Correlation=source.Correlation;
    header.PayloadBytes=1;
    assert(Sockets::SocketAdapterWire::EncodeHeader(header,receipt.data(),receipt.size()));
    receipt[Sockets::SocketAdapterWire::HeaderBytes]=static_cast<std::uint8_t>(admission);
    return receipt;
}

void BindStart(Transport& transport,Wire& wire,Adapters::AdapterRouteToken route,
               const Adapters::AdapterSemanticProvenance& provenance) {
    assert(transport.BindSession(route,Sockets::SocketAdapterSessionMode::Datagram,
        {&wire,&Wire::Write},provenance,true));
    assert(transport.Freeze());
    assert(transport.Start());
}

} // namespace

int main() {
    int resolverOwner=0;
    std::size_t wakes=0;
    Adapters::AdapterSemanticProvenance provenance{};
    provenance.ImmediatePeer.Token=0x44;
    const Adapters::AdapterRouteToken route{1};

    Primitive::PrimitivePolicyDescriptor withEvidence{};
    withEvidence.Category=1;
    withEvidence.Evidence=1;
    withEvidence.MaximumAttempts=1;
    const std::uint8_t evidencePayload[]{1,0x5A};

    // A live destination-admission attempt must survive every single-bit mutation
    // of the receipt header. No corrupted header may complete the current record.
    FakeAdapter adapter;
    Wire wire;
    Transport transport(adapter,{&resolverOwner,&ResolvePolicy},{&wakes,&Wake});
    BindStart(transport,wire,route,provenance);
    auto submission=transport.Submit(
        Record(1),1,1,withEvidence,Adapters::AdapterServiceClass::Responsive,
        {evidencePayload,sizeof(evidencePayload)},route);
    assert(submission.Disposition==Adapters::LowerTransportDisposition::Accepted);
    assert(submission.DeferredCompletion);
    const auto receipt=BuildReceipt(wire.Bytes,Primitive::PrimitiveAdmissionDisposition::Accepted);

    for(std::size_t byte=0;byte<Sockets::SocketAdapterWire::HeaderBytes;++byte) {
        for(std::uint8_t bit=0;bit<8;++bit) {
            auto corrupted=receipt;
            corrupted[byte]=static_cast<std::uint8_t>(corrupted[byte]^(std::uint8_t{1}<<bit));
            const auto fed=transport.FeedDatagram(route,corrupted.data(),corrupted.size());
            assert(fed.Status!=Sockets::SocketAdapterFeedStatus::Accepted);
            assert(adapter.CompleteCount==0);
        }
    }

    // Admission bytes outside the exact seven-value M1 domain are malformed and
    // must not consume the still-live correlation slot.
    for(unsigned raw=static_cast<unsigned>(Primitive::PrimitiveAdmissionDisposition::Malformed)+1;raw<=0xFFu;++raw) {
        auto malformed=receipt;
        malformed[Sockets::SocketAdapterWire::HeaderBytes]=static_cast<std::uint8_t>(raw);
        const auto fed=transport.FeedDatagram(route,malformed.data(),malformed.size());
        assert(fed.Status==Sockets::SocketAdapterFeedStatus::Malformed);
        assert(adapter.CompleteCount==0);
    }

    assert(transport.FeedDatagram(route,receipt.data(),receipt.size()).Status==Sockets::SocketAdapterFeedStatus::Accepted);
    assert(transport.ServiceOne());
    assert(adapter.CompleteCount==1);
    assert(adapter.LastCompletion.HasDestinationAdmission);
    assert(adapter.LastCompletion.DestinationAdmission==Primitive::PrimitiveAdmissionDisposition::Accepted);
    assert(Primitive::EstablishesDestinationAdmission(adapter.LastCompletion.DestinationAdmission));

    // Every exact non-success M1 disposition is transportable but must remain
    // semantically distinct from DestinationPrimitiveAdmission.
    const std::array<Primitive::PrimitiveAdmissionDisposition,5> nonSuccess{
        Primitive::PrimitiveAdmissionDisposition::TemporarilyUnavailable,
        Primitive::PrimitiveAdmissionDisposition::ResourceUnavailable,
        Primitive::PrimitiveAdmissionDisposition::Unsupported,
        Primitive::PrimitiveAdmissionDisposition::Rejected,
        Primitive::PrimitiveAdmissionDisposition::Malformed
    };
    std::uint64_t recordGeneration=10;
    for(const auto admission:nonSuccess) {
        submission=transport.Submit(
            Record(recordGeneration++),1,1,withEvidence,Adapters::AdapterServiceClass::Responsive,
            {evidencePayload,sizeof(evidencePayload)},route);
        assert(submission.Disposition==Adapters::LowerTransportDisposition::Accepted&&submission.DeferredCompletion);
        const auto nonSuccessReceipt=BuildReceipt(wire.Bytes,admission);
        assert(transport.FeedDatagram(route,nonSuccessReceipt.data(),nonSuccessReceipt.size()).Status==Sockets::SocketAdapterFeedStatus::Accepted);
        assert(transport.ServiceOne());
        assert(adapter.LastCompletion.HasDestinationAdmission);
        assert(adapter.LastCompletion.DestinationAdmission==admission);
        assert(!Primitive::EstablishesDestinationAdmission(admission));
    }

    // Session-generation churn invalidates every old receipt. Reconnection must
    // not let an old correlation become current even after repeated availability cycles.
    submission=transport.Submit(
        Record(100),1,1,withEvidence,Adapters::AdapterServiceClass::Responsive,
        {evidencePayload,sizeof(evidencePayload)},route);
    assert(submission.DeferredCompletion);
    const auto staleReceipt=BuildReceipt(wire.Bytes,Primitive::PrimitiveAdmissionDisposition::Accepted);
    for(unsigned cycle=0;cycle<64;++cycle) {
        assert(transport.SetSessionAvailable(route,false));
        assert(transport.SetSessionAvailable(route,true));
        assert(transport.FeedDatagram(route,staleReceipt.data(),staleReceipt.size()).Status==Sockets::SocketAdapterFeedStatus::Rejected);
    }
    assert(transport.ServiceOne());
    assert(adapter.LastCompletion.Disposition==Adapters::LowerTransportDisposition::TemporarilyUnavailable);
    assert(!adapter.LastCompletion.HasDestinationAdmission);

    // Stream assembly remains bounded and recovers after malformed input. Feed a
    // valid frame in deterministic pseudo-random chunk sizes to exercise every
    // incremental-header/payload boundary without an unbounded parser loop.
    Primitive::PrimitivePolicyDescriptor noEvidence=withEvidence;
    noEvidence.Evidence=0;
    const std::uint8_t noEvidencePayload[]{0,0xA5,0x5A,0x11,0x22};
    FakeAdapter senderAdapter,streamAdapter;
    Wire senderWire,streamWire;
    Transport sender(senderAdapter,{&resolverOwner,&ResolvePolicy},{&wakes,&Wake});
    Transport stream(streamAdapter,{&resolverOwner,&ResolvePolicy},{&wakes,&Wake});
    BindStart(sender,senderWire,{2},provenance);
    assert(stream.BindSession({2},Sockets::SocketAdapterSessionMode::Stream,{&streamWire,&Wire::Write},provenance,true));
    assert(stream.Freeze()&&stream.Start());
    auto streamSubmission=sender.Submit(
        Record(200),1,1,noEvidence,Adapters::AdapterServiceClass::BestEffort,
        {noEvidencePayload,sizeof(noEvidencePayload)},{2});
    assert(streamSubmission.Disposition==Adapters::LowerTransportDisposition::Accepted);

    std::array<std::uint8_t,Sockets::SocketAdapterWire::HeaderBytes> malformedHeader{};
    std::size_t malformedOffset=0;
    while(malformedOffset<malformedHeader.size()) {
        const std::size_t chunk=(malformedOffset%5u)+1u;
        const std::size_t remaining=malformedHeader.size()-malformedOffset;
        const std::size_t actual=chunk<remaining?chunk:remaining;
        const auto fed=stream.FeedStream({2},malformedHeader.data()+malformedOffset,actual);
        malformedOffset+=fed.Consumed;
        if(fed.Status==Sockets::SocketAdapterFeedStatus::Malformed) break;
        assert(fed.Status==Sockets::SocketAdapterFeedStatus::Partial);
    }

    std::size_t offset=0;
    std::uint32_t prng=0xC0FFEEu;
    while(offset<senderWire.Bytes.size()) {
        prng=prng*1664525u+1013904223u;
        std::size_t chunk=1u+(prng%11u);
        const std::size_t remaining=senderWire.Bytes.size()-offset;
        if(chunk>remaining) chunk=remaining;
        const auto fed=stream.FeedStream({2},senderWire.Bytes.data()+offset,chunk);
        assert(fed.Consumed>0&&fed.Consumed<=chunk);
        offset+=fed.Consumed;
        if(offset<senderWire.Bytes.size())
            assert(fed.Status==Sockets::SocketAdapterFeedStatus::Partial);
        else
            assert(fed.Status==Sockets::SocketAdapterFeedStatus::Accepted);
    }
    assert(streamAdapter.AdmitCount==1);

    return 0;
}
