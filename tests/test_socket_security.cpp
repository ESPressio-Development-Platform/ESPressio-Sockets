#include <cassert>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <vector>

#include <ESPressio_Security.hpp>
#include "ESPressio_SocketSecurityDatagram.hpp"
#include "ESPressio_SocketSecuritySession.hpp"

using namespace ESPressio;

class Random final : public Security::IRandomSource {
public:
    bool Fill(uint8_t* out, std::size_t size) override { for (std::size_t i=0;i<size;++i) out[i]=static_cast<uint8_t>(++_next); return true; }
private: uint8_t _next = 0;
};

class TestCipher final : public Security::IAeadCipher {
public:
    Security::AeadAlgorithm Algorithm() const noexcept override { return Security::AeadAlgorithm::TestOnly; }
    const char* Name() const noexcept override { return "TEST"; }
    std::size_t KeySize() const noexcept override { return 16; }
    std::size_t NonceSize() const noexcept override { return 12; }
    std::size_t TagSize() const noexcept override { return 16; }
    bool Seal(const uint8_t* key,std::size_t ks,const uint8_t* nonce,std::size_t ns,const uint8_t* aad,std::size_t as,const uint8_t* plain,std::size_t ps,std::vector<uint8_t>& ct,std::vector<uint8_t>& tag) override {
        if(ks!=16||ns!=12)return false; ct.resize(ps); uint8_t h=0; for(std::size_t i=0;i<as;++i)h^=aad[i];
        for(std::size_t i=0;i<ps;++i){ct[i]=plain[i]^key[i%ks]^nonce[i%ns];h^=ct[i];} tag.assign(16,h); return true;
    }
    bool Open(const uint8_t* key,std::size_t ks,const uint8_t* nonce,std::size_t ns,const uint8_t* aad,std::size_t as,const uint8_t* ct,std::size_t cs,const uint8_t* tag,std::size_t ts,std::vector<uint8_t>& plain) override {
        if(ks!=16||ns!=12||ts!=16)return false; uint8_t h=0; for(std::size_t i=0;i<as;++i)h^=aad[i]; for(std::size_t i=0;i<cs;++i)h^=ct[i]; for(std::size_t i=0;i<ts;++i)if(tag[i]!=h)return false;
        plain.resize(cs); for(std::size_t i=0;i<cs;++i)plain[i]=ct[i]^key[i%ks]^nonce[i%ns]; return true;
    }
};

static Security::TransportSecurity Make(Security::AeadCipherRegistry& registry, Security::StaticKeyProvider& keys, Random& random, uint64_t sender, Security::TransportSecurityPolicy policy = Security::TransportSecurityPolicy::Required) {
    Security::TransportSecurityConfig cfg; cfg.Policy=policy; cfg.OutboundAlgorithm=Security::AeadAlgorithm::TestOnly; cfg.OutboundKeyID=1; cfg.SenderID=sender; cfg.MaximumPlaintextBytes=4096;
    return Security::TransportSecurity(registry,keys,random,cfg);
}

int main() {
    TestCipher cipher; Security::AeadCipherRegistry registry; assert(registry.Register(cipher));
    Security::StaticKeyProvider keys; uint8_t key[16]={1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16}; assert(keys.Add(1,Security::AeadAlgorithm::TestOnly,key,16));
    Random randomA, randomB; auto sender=Make(registry,keys,randomA,11); auto receiver=Make(registry,keys,randomB,22);

    std::vector<uint8_t> wire;
    Sockets::SocketSecuritySession tx(sender,[&](const uint8_t* data,std::size_t size){wire.assign(data,data+size);return true;});
    Sockets::SocketSecuritySession rx(receiver,[](const uint8_t*,std::size_t){return true;});
    bool received=false; rx.SetReceiveCallback([&](const Security::UnprotectedPayload& opened){received=true;assert(opened.Protocol==9);assert(std::string(opened.Data.begin(),opened.Data.end())=="hello stream");});
    assert(tx.Send(9,"hello stream",12)); assert(!wire.empty());
    assert(wire[4] == 9); // outer route field
    // Arbitrary TCP-style chunking.
    assert(rx.Feed(wire.data(),2)); assert(!received); assert(rx.Feed(wire.data()+2,5)); assert(!received); assert(rx.Feed(wire.data()+7,wire.size()-7)); assert(received);

    // Multiple framed messages in one receive call.
    std::vector<uint8_t> first=wire, second; assert(tx.Send(9,"hello stream",12)); second=wire; first.insert(first.end(),second.begin(),second.end()); int count=0;
    auto receiver2=Make(registry,keys,randomB,33); Sockets::SocketSecuritySession rx2(receiver2,[](const uint8_t*,std::size_t){return true;}); rx2.SetReceiveCallback([&](const Security::UnprotectedPayload&){++count;}); assert(rx2.Feed(first.data(),first.size())); assert(count==2);

    // Oversized declared stream frame is rejected and session can be Reset.
    auto receiver3=Make(registry,keys,randomB,44); Sockets::SocketSecuritySessionConfig tiny; tiny.MaximumProtectedFrameBytes=64; Sockets::SocketSecuritySession limited(receiver3,[](const uint8_t*,std::size_t){return true;},tiny); uint8_t bad[5]={0xFF,0x00,0x00,0x00,0x09}; assert(!limited.Feed(bad,5)); limited.Reset(); assert(limited.BufferedBytes()==0);

    // Datagram path preserves one complete Security envelope plus outer protocol field.
    std::vector<uint8_t> packet; auto ds=Make(registry,keys,randomA,55); auto dr=Make(registry,keys,randomB,66);
    Sockets::SocketSecurityDatagram dtx(ds,[&](const uint8_t* d,std::size_t s){packet.assign(d,d+s);return true;});
    Sockets::SocketSecurityDatagram drx(dr,[](const uint8_t*,std::size_t){return true;}); bool dgot=false; drx.SetReceiveCallback([&](const Security::UnprotectedPayload& p){dgot=true;assert(p.Protocol==7);assert(p.Protected);});
    assert(dtx.Send(7,"udp",3)); assert(packet.front()==7); assert(drx.Receive(packet.data(),packet.size())); assert(dgot);
    // Replay of same datagram is rejected by Security.
    assert(!drx.Receive(packet.data(),packet.size()));

    // Disabled policy still preserves routing because protocol lives outside the optional envelope.
    auto plainSender=Make(registry,keys,randomA,77,Security::TransportSecurityPolicy::Disabled);
    auto plainReceiver=Make(registry,keys,randomB,88,Security::TransportSecurityPolicy::Disabled);
    std::vector<uint8_t> plainWire;
    Sockets::SocketSecuritySession ptx(plainSender,[&](const uint8_t* d,std::size_t s){plainWire.assign(d,d+s);return true;});
    Sockets::SocketSecuritySession prx(plainReceiver,[](const uint8_t*,std::size_t){return true;});
    bool pgot=false; prx.SetReceiveCallback([&](const Security::UnprotectedPayload& p){pgot=true;assert(p.Protocol==12);assert(!p.Protected);assert(std::string(p.Data.begin(),p.Data.end())=="plain");});
    assert(ptx.Send(12,"plain",5)); assert(plainWire[4]==12); assert(prx.Feed(plainWire.data(),plainWire.size())); assert(pgot);

    // Tampering only the outer route field of a protected frame causes authenticated protocol mismatch.
    auto tampered=wire; tampered[4]=10;
    auto receiver4=Make(registry,keys,randomB,99); Sockets::SocketSecuritySession rx4(receiver4,[](const uint8_t*,std::size_t){return true;});
    bool failed=false; rx4.SetFailureCallback([&](const Security::SecurityResult& r){failed=true;assert(r.Error==Security::SecurityError::ProtocolMismatch);});
    assert(rx4.Feed(tampered.data(),tampered.size())); assert(failed);

    std::cout << "Socket Security tests passed\n";
}
