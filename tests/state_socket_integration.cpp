#include <cassert>
#include <cstdint>
#include <cstdio>
#include <vector>
#include <ESPressio_State.hpp>
#include "ESPressio_SocketStateSession.hpp"
namespace {
struct Temperature { using Value=int32_t; static constexpr ESPressio::State::StateTypeId Id=0x53544354454D5001ULL; };
struct Enabled { using Value=bool; static constexpr ESPressio::State::StateTypeId Id=0x535443454E414201ULL; };
using Contract=ESPressio::State::StateContract<Temperature,Enabled>; using Publisher=ESPressio::State::StatePublisher<Contract>; using Remote=ESPressio::State::RemoteStateManager<Contract,2>; using Subscriptions=ESPressio::State::StateSubscriptionRegistry<4>; using Session=ESPressio::Sockets::SocketStateSession<Contract,2,4>;
ESPressio::State::DeviceIdentifier MakeDevice(uint8_t tail){ ESPressio::State::DeviceIdentifier::Storage bytes{}; bytes[15]=tail; return ESPressio::State::DeviceIdentifier(bytes); }
void Checkpoint(const char* text){ std::fprintf(stderr,"%s\n",text); std::fflush(stderr); }
}
int main(){ using namespace ESPressio; int32_t temperatureA=21,temperatureB=32; bool enabledA=true,enabledB=false; const auto deviceA=MakeDevice(0xA1),deviceB=MakeDevice(0xB2); Publisher publisherA(deviceA),publisherB(deviceB); assert(publisherA.RegisterSource<Temperature>([&]{return temperatureA;})); assert(publisherA.RegisterSource<Enabled>([&]{return enabledA;})); assert(publisherB.RegisterSource<Temperature>([&]{return temperatureB;})); assert(publisherB.RegisterSource<Enabled>([&]{return enabledB;})); Remote remoteA,remoteB; Subscriptions subscriptionsA,subscriptionsB; Session sessionA,sessionB; Sockets::SocketStateSessionConfig config; config.MaximumProtocolMessageBytes=256;
Checkpoint("initialize A"); assert(sessionA.Initialize(publisherA,remoteA,subscriptionsA,config,[&](const uint8_t* d,std::size_t n){ Checkpoint("A -> B enter"); const bool ok=sessionB.Feed(d,n); Checkpoint("A -> B return"); return ok; })); Checkpoint("initialize B"); assert(sessionB.Initialize(publisherB,remoteB,subscriptionsB,config,[&](const uint8_t* d,std::size_t n){ Checkpoint("B -> A enter"); const bool ok=sessionA.Feed(d,n); Checkpoint("B -> A return"); return ok; }));
Checkpoint("subscribe B->A temperature"); assert(subscriptionsB.Subscribe<Temperature>()); Checkpoint("subscribed B->A temperature"); assert(sessionA.RemoteDevice()==deviceB && sessionB.RemoteDevice()==deviceA); State::RemoteStateSnapshot<int32_t> ts; assert(remoteB.Read<Temperature>(deviceA,ts)&&ts.HasValue&&ts.Value==21);
temperatureA=27; Checkpoint("publish A temperature"); assert(publisherA.Publish<Temperature>()); assert(remoteB.Read<Temperature>(deviceA,ts)&&ts.Value==27);
Checkpoint("subscribe A->B enabled"); assert(subscriptionsA.Subscribe<Enabled>()); State::RemoteStateSnapshot<bool> es; assert(remoteA.Read<Enabled>(deviceB,es)&&es.HasValue&&!es.Value); enabledB=true; Checkpoint("publish B enabled"); assert(publisherB.Publish<Enabled>()); assert(remoteA.Read<Enabled>(deviceB,es)&&es.Value);
Checkpoint("unsubscribe B temperature"); assert(subscriptionsB.Unsubscribe<Temperature>()); temperatureA=41; assert(publisherA.Publish<Temperature>()); assert(remoteB.Read<Temperature>(deviceA,ts)&&ts.Value==27);
std::vector<uint8_t> delivered; Sockets::SocketStateFrameDecoder decoder(64); const uint8_t protocol[]={1,2,3,4,5}; auto frame=Sockets::BuildSocketStateFrame(protocol,sizeof(protocol),64); assert(!frame.empty()); assert(decoder.Push(frame.data(),3,[&](const uint8_t*,std::size_t){assert(false);})); assert(decoder.Push(frame.data()+3,frame.size()-3,[&](const uint8_t* d,std::size_t n){delivered.assign(d,d+n);})); assert(delivered==std::vector<uint8_t>(protocol,protocol+sizeof(protocol)));
Checkpoint("shutdown A"); sessionA.Shutdown(); Checkpoint("shutdown B"); sessionB.Shutdown(); Checkpoint("done"); return 0; }
