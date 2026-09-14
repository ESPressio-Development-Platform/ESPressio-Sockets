#include <ESPressio_SocketAdapterTransport.hpp>

#include <array>
#include <cassert>
#include <cstdint>
#include <vector>

using namespace ESPressio;

struct FakeAdapter final {
    Adapters::AdapterInboundCompletionTarget InboundCompletion{};
    std::uint64_t InboundCorrelation=0;
    std::size_t AdmitCount=0;
    std::size_t CompleteCount=0;
    Adapters::LowerTransportCompletion LastCompletion{};
    Adapters::AdapterSubmissionDisposition NextAdmission=Adapters::AdapterSubmissionDisposition::Accepted;

    Adapters::AdapterSubmissionDisposition AdmitTrustedInbound(
        Primitive::PrimitiveFamilyId, Adapters::AdapterServiceClass,
        Primitive::PrimitiveProtocolVersion, Adapters::AdapterByteView,
        const Adapters::AdapterSemanticProvenance&, Adapters::AdapterRouteToken,
        Primitive::PrimitivePolicyDescriptor, std::uint64_t correlation,
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

static Adapters::AdapterRecordIdentity Record(std::uint64_t generation) {
    Adapters::AdapterRecordIdentity result{};
    result.Direction=Adapters::AdapterDirection::Outbound;
    result.Generation=generation;
    return result;
}

int main() {
    using Transport=Sockets::SocketAdapterTransport<FakeAdapter,1,1,1,128>;
    int resolverOwner=0;
    std::size_t wakes=0;
    FakeAdapter leftAdapter,rightAdapter;
    Wire leftWire,rightWire;
    Transport left(leftAdapter,{&resolverOwner,&ResolvePolicy},{&wakes,&Wake});
    Transport right(rightAdapter,{&resolverOwner,&ResolvePolicy},{&wakes,&Wake});
    Adapters::AdapterSemanticProvenance provenance{};
    provenance.ImmediatePeer.Token=7;

    assert(left.BindSession({1},Sockets::SocketAdapterSessionMode::Datagram,{&leftWire,&Wire::Write},provenance,true));
    assert(right.BindSession({1},Sockets::SocketAdapterSessionMode::Datagram,{&rightWire,&Wire::Write},provenance,true));
    assert(left.Freeze()&&right.Freeze());
    assert(left.Start()&&right.Start());

    Primitive::PrimitivePolicyDescriptor noEvidence{};
    noEvidence.Category=1;
    noEvidence.Evidence=0;
    noEvidence.MaximumAttempts=1;
    const std::uint8_t noEvidencePayload[]{0,0xA5};
    auto immediate=left.Submit(
        Record(1),1,1,noEvidence,Adapters::AdapterServiceClass::BestEffort,
        {noEvidencePayload,sizeof(noEvidencePayload)},{1});
    assert(immediate.Disposition==Adapters::LowerTransportDisposition::Accepted);
    assert(!immediate.DeferredCompletion);
    assert(leftWire.Bytes.size()==Sockets::SocketAdapterWire::HeaderBytes+sizeof(noEvidencePayload));
    assert(Sockets::SocketAdapterWire::Read32(leftWire.Bytes.data())==Sockets::SocketAdapterWire::Magic);
    assert(leftWire.Bytes[4]==Sockets::SocketAdapterWire::Version);
    assert(leftWire.Bytes[5]==static_cast<std::uint8_t>(Sockets::SocketAdapterFrameKind::Primitive));
    assert(leftWire.Bytes[6]==static_cast<std::uint8_t>(Adapters::AdapterServiceClass::BestEffort));
    assert(Sockets::SocketAdapterWire::Read16(leftWire.Bytes.data()+8)==1);
    assert(Sockets::SocketAdapterWire::Read16(leftWire.Bytes.data()+10)==1);
    assert(Sockets::SocketAdapterWire::Read64(leftWire.Bytes.data()+12)==0);
    assert(right.FeedDatagram({1},leftWire.Bytes.data(),leftWire.Bytes.size()).Status==Sockets::SocketAdapterFeedStatus::Accepted);

    Primitive::PrimitivePolicyDescriptor withEvidence=noEvidence;
    withEvidence.Evidence=1;
    const std::uint8_t evidencePayload[]{1,0x5A};
    auto deferred=left.Submit(
        Record(2),1,1,withEvidence,Adapters::AdapterServiceClass::Responsive,
        {evidencePayload,sizeof(evidencePayload)},{1});
    assert(deferred.Disposition==Adapters::LowerTransportDisposition::Accepted);
    assert(deferred.DeferredCompletion);
    auto exhausted=left.Submit(
        Record(3),1,1,withEvidence,Adapters::AdapterServiceClass::Responsive,
        {evidencePayload,sizeof(evidencePayload)},{1});
    assert(exhausted.Disposition==Adapters::LowerTransportDisposition::ResourceUnavailable);
    assert(right.FeedDatagram({1},leftWire.Bytes.data(),leftWire.Bytes.size()).Status==Sockets::SocketAdapterFeedStatus::Accepted);
    assert(rightAdapter.InboundCompletion.Owner!=nullptr&&rightAdapter.InboundCompletion.Complete!=nullptr);
    Adapters::AdapterInboundCompletion admitted{};
    admitted.Correlation=rightAdapter.InboundCorrelation;
    admitted.Admission=Primitive::PrimitiveAdmissionDisposition::Accepted;
    rightAdapter.InboundCompletion.Complete(rightAdapter.InboundCompletion.Owner,admitted);
    assert(right.ServiceOne());
    assert(left.FeedDatagram({1},rightWire.Bytes.data(),rightWire.Bytes.size()).Status==Sockets::SocketAdapterFeedStatus::Accepted);
    assert(left.ServiceOne());
    assert(leftAdapter.CompleteCount==1&&leftAdapter.LastCompletion.HasDestinationAdmission);
    assert(leftAdapter.LastCompletion.DestinationAdmission==Primitive::PrimitiveAdmissionDisposition::Accepted);

    auto pending=left.Submit(
        Record(4),1,1,withEvidence,Adapters::AdapterServiceClass::Responsive,
        {evidencePayload,sizeof(evidencePayload)},{1});
    assert(pending.DeferredCompletion);
    const auto staleFrame=leftWire.Bytes;
    assert(left.SetSessionAvailable({1},false));
    assert(left.ServiceOne());
    assert(leftAdapter.CompleteCount==2&&!leftAdapter.LastCompletion.HasDestinationAdmission);
    assert(leftAdapter.LastCompletion.Disposition==Adapters::LowerTransportDisposition::TemporarilyUnavailable);
    assert(left.SetSessionAvailable({1},true));

    Sockets::SocketAdapterWire::Header old{};
    assert(Sockets::SocketAdapterWire::DecodeHeader(staleFrame.data(),staleFrame.size(),old));
    std::array<std::uint8_t,128> staleReceipt{};
    Sockets::SocketAdapterWire::Header receiptHeader{};
    receiptHeader.Kind=Sockets::SocketAdapterFrameKind::AdmissionReceipt;
    receiptHeader.Service=old.Service;
    receiptHeader.Family=old.Family;
    receiptHeader.Protocol=old.Protocol;
    receiptHeader.Correlation=old.Correlation;
    receiptHeader.PayloadBytes=1;
    assert(Sockets::SocketAdapterWire::EncodeHeader(receiptHeader,staleReceipt.data(),staleReceipt.size()));
    staleReceipt[Sockets::SocketAdapterWire::HeaderBytes]=
        static_cast<std::uint8_t>(Primitive::PrimitiveAdmissionDisposition::Accepted);
    assert(left.FeedDatagram({1},staleReceipt.data(),Sockets::SocketAdapterWire::HeaderBytes+1).Status==Sockets::SocketAdapterFeedStatus::Rejected);

    const auto generation=left.Generation();
    left.Quiesce();
    assert(!left.IsActive());
    auto quiescedSubmit=left.Submit(
        Record(5),1,1,withEvidence,Adapters::AdapterServiceClass::Responsive,
        {evidencePayload,sizeof(evidencePayload)},{1});
    assert(quiescedSubmit.Disposition==Adapters::LowerTransportDisposition::TemporarilyUnavailable);
    assert(!quiescedSubmit.DeferredCompletion);
    assert(left.FeedDatagram({1},staleReceipt.data(),Sockets::SocketAdapterWire::HeaderBytes+1).Status==Sockets::SocketAdapterFeedStatus::NotRunning);
    assert(left.Restart(leftAdapter));
    assert(left.Generation()==generation+1);
    assert(left.FeedDatagram({1},staleReceipt.data(),Sockets::SocketAdapterWire::HeaderBytes+1).Status==Sockets::SocketAdapterFeedStatus::Rejected);

    auto staleInboundSend=left.Submit(
        Record(6),1,1,withEvidence,Adapters::AdapterServiceClass::Responsive,
        {evidencePayload,sizeof(evidencePayload)},{1});
    assert(staleInboundSend.Disposition==Adapters::LowerTransportDisposition::Accepted);
    assert(staleInboundSend.DeferredCompletion);
    assert(right.FeedDatagram({1},leftWire.Bytes.data(),leftWire.Bytes.size()).Status==Sockets::SocketAdapterFeedStatus::Accepted);
    const auto staleInboundTarget=rightAdapter.InboundCompletion;
    const auto staleInboundCorrelation=rightAdapter.InboundCorrelation;
    const auto rightGeneration=right.Generation();
    right.Quiesce();
    assert(!right.IsActive());
    assert(right.Restart(rightAdapter));
    assert(right.Generation()==rightGeneration+1);
    Adapters::AdapterInboundCompletion staleInbound{};
    staleInbound.Correlation=staleInboundCorrelation;
    staleInbound.Admission=Primitive::PrimitiveAdmissionDisposition::Accepted;
    staleInboundTarget.Complete(staleInboundTarget.Owner,staleInbound);
    assert(!right.ServiceOne());
    left.Cancel(Record(6));

    FakeAdapter streamAdapter;
    Wire streamWire;
    Transport stream(streamAdapter,{&resolverOwner,&ResolvePolicy},{&wakes,&Wake});
    assert(stream.BindSession({2},Sockets::SocketAdapterSessionMode::Stream,{&streamWire,&Wire::Write},provenance,true));
    assert(stream.Freeze());
    assert(stream.Start());
    auto streamSend=stream.Submit(
        Record(7),1,1,noEvidence,Adapters::AdapterServiceClass::BestEffort,
        {noEvidencePayload,sizeof(noEvidencePayload)},{2});
    assert(streamSend.Disposition==Adapters::LowerTransportDisposition::Accepted);
    auto first=stream.FeedStream({2},streamWire.Bytes.data(),10);
    assert(first.Status==Sockets::SocketAdapterFeedStatus::Partial&&first.Consumed==10);
    auto second=stream.FeedStream({2},streamWire.Bytes.data()+10,streamWire.Bytes.size()-10);
    assert(second.Status==Sockets::SocketAdapterFeedStatus::Accepted);
    assert(streamAdapter.AdmitCount==1);
    return 0;
}
