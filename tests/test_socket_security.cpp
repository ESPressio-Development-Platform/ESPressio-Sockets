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
    bool Fill(uint8_t* out, std::size_t size) override {
        for (std::size_t i = 0; i < size; ++i) {
            out[i] = static_cast<uint8_t>(++_next);
        }
        return true;
    }

private:
    uint8_t _next = 0;
};

class TestCipher final : public Security::IAeadCipher {
public:
    Security::AeadAlgorithm Algorithm() const noexcept override { return Security::AeadAlgorithm::TestOnly; }
    const char* Name() const noexcept override { return "TEST"; }
    std::size_t KeySize() const noexcept override { return 16; }
    std::size_t NonceSize() const noexcept override { return 12; }
    std::size_t TagSize() const noexcept override { return 16; }

    bool Seal(
        const uint8_t* key,
        std::size_t keySize,
        const uint8_t* nonce,
        std::size_t nonceSize,
        const uint8_t* aad,
        std::size_t aadSize,
        const uint8_t* plaintext,
        std::size_t plaintextSize,
        std::vector<uint8_t>& ciphertext,
        std::vector<uint8_t>& tag
    ) override {
        if (keySize != 16 || nonceSize != 12) {
            return false;
        }

        ciphertext.resize(plaintextSize);
        uint8_t hash = 0;
        for (std::size_t i = 0; i < aadSize; ++i) {
            hash ^= aad[i];
        }
        for (std::size_t i = 0; i < plaintextSize; ++i) {
            ciphertext[i] = plaintext[i] ^ key[i % keySize] ^ nonce[i % nonceSize];
            hash ^= ciphertext[i];
        }
        tag.assign(16, hash);
        return true;
    }

    bool Open(
        const uint8_t* key,
        std::size_t keySize,
        const uint8_t* nonce,
        std::size_t nonceSize,
        const uint8_t* aad,
        std::size_t aadSize,
        const uint8_t* ciphertext,
        std::size_t ciphertextSize,
        const uint8_t* tag,
        std::size_t tagSize,
        std::vector<uint8_t>& plaintext
    ) override {
        if (keySize != 16 || nonceSize != 12 || tagSize != 16) {
            return false;
        }

        uint8_t hash = 0;
        for (std::size_t i = 0; i < aadSize; ++i) {
            hash ^= aad[i];
        }
        for (std::size_t i = 0; i < ciphertextSize; ++i) {
            hash ^= ciphertext[i];
        }
        for (std::size_t i = 0; i < tagSize; ++i) {
            if (tag[i] != hash) {
                return false;
            }
        }

        plaintext.resize(ciphertextSize);
        for (std::size_t i = 0; i < ciphertextSize; ++i) {
            plaintext[i] = ciphertext[i] ^ key[i % keySize] ^ nonce[i % nonceSize];
        }
        return true;
    }
};

static Security::TransportSecurity Make(
    Security::AeadCipherRegistry& registry,
    Security::StaticKeyProvider& keys,
    Random& random,
    uint64_t sender,
    Security::TransportSecurityPolicy policy = Security::TransportSecurityPolicy::Required
) {
    Security::TransportSecurityConfig config;
    config.Policy = policy;
    config.OutboundAlgorithm = Security::AeadAlgorithm::TestOnly;
    config.OutboundKeyID = 1;
    config.SenderID = sender;
    config.MaximumPlaintextBytes = 4096;
    return Security::TransportSecurity(registry, keys, random, config);
}

int main() {
    TestCipher cipher;
    Security::AeadCipherRegistry registry;
    assert(registry.Register(cipher));

    Security::StaticKeyProvider keys;
    uint8_t key[16] = {1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16};
    assert(keys.Add(1, Security::AeadAlgorithm::TestOnly, key, 16));

    Random randomA, randomB;
    auto sender = Make(registry, keys, randomA, 11);
    auto receiver = Make(registry, keys, randomB, 22);

    std::vector<uint8_t> wire;
    Sockets::SocketSecuritySession tx(sender, [&](const uint8_t* data, std::size_t size) {
        wire.assign(data, data + size);
        return true;
    });
    Sockets::SocketSecuritySession rx(receiver, [](const uint8_t*, std::size_t) { return true; });

    bool received = false;
    rx.SetReceiveCallback([&](const Security::UnprotectedPayload& opened) {
        received = true;
        assert(opened.Protocol == 9);
        assert(std::string(opened.Data.begin(), opened.Data.end()) == "hello stream");
    });

    assert(tx.Send(9, "hello stream", 12));
    assert(!wire.empty());
    assert(wire[4] == 9);

    assert(rx.Feed(wire.data(), 2));
    assert(!received);
    assert(rx.Feed(wire.data() + 2, 5));
    assert(!received);
    assert(rx.Feed(wire.data() + 7, wire.size() - 7));
    assert(received);

    std::vector<uint8_t> first = wire;
    std::vector<uint8_t> second;
    assert(tx.Send(9, "hello stream", 12));
    second = wire;
    first.insert(first.end(), second.begin(), second.end());

    int count = 0;
    auto receiver2 = Make(registry, keys, randomB, 33);
    Sockets::SocketSecuritySession rx2(receiver2, [](const uint8_t*, std::size_t) { return true; });
    rx2.SetReceiveCallback([&](const Security::UnprotectedPayload&) { ++count; });
    assert(rx2.Feed(first.data(), first.size()));
    assert(count == 2);

    auto receiver3 = Make(registry, keys, randomB, 44);
    Sockets::SocketSecuritySessionConfig tiny;
    tiny.MaximumProtectedFrameBytes = 64;
    Sockets::SocketSecuritySession limited(receiver3, [](const uint8_t*, std::size_t) { return true; }, tiny);
    uint8_t bad[5] = {0xFF,0x00,0x00,0x00,0x09};
    assert(!limited.Feed(bad, 5));
    limited.Reset();
    assert(limited.BufferedBytes() == 0);

    std::vector<uint8_t> packet;
    auto datagramSender = Make(registry, keys, randomA, 55);
    auto datagramReceiver = Make(registry, keys, randomB, 66);
    Sockets::SocketSecurityDatagram dtx(datagramSender, [&](const uint8_t* data, std::size_t size) {
        packet.assign(data, data + size);
        return true;
    });
    Sockets::SocketSecurityDatagram drx(datagramReceiver, [](const uint8_t*, std::size_t) { return true; });

    bool datagramReceived = false;
    drx.SetReceiveCallback([&](const Security::UnprotectedPayload& payload) {
        datagramReceived = true;
        assert(payload.Protocol == 7);
        assert(payload.Protected);
    });

    assert(dtx.Send(7, "udp", 3));
    assert(packet.front() == 7);
    assert(drx.Receive(packet.data(), packet.size()));
    assert(datagramReceived);
    assert(!drx.Receive(packet.data(), packet.size()));

    auto plainSender = Make(registry, keys, randomA, 77, Security::TransportSecurityPolicy::Disabled);
    auto plainReceiver = Make(registry, keys, randomB, 88, Security::TransportSecurityPolicy::Disabled);
    std::vector<uint8_t> plainWire;
    Sockets::SocketSecuritySession plainTx(plainSender, [&](const uint8_t* data, std::size_t size) {
        plainWire.assign(data, data + size);
        return true;
    });
    Sockets::SocketSecuritySession plainRx(plainReceiver, [](const uint8_t*, std::size_t) { return true; });

    bool plainReceived = false;
    plainRx.SetReceiveCallback([&](const Security::UnprotectedPayload& payload) {
        plainReceived = true;
        assert(payload.Protocol == 12);
        assert(!payload.Protected);
        assert(std::string(payload.Data.begin(), payload.Data.end()) == "plain");
    });

    assert(plainTx.Send(12, "plain", 5));
    assert(plainWire[4] == 12);
    assert(plainRx.Feed(plainWire.data(), plainWire.size()));
    assert(plainReceived);

    auto tampered = wire;
    tampered[4] = 10;
    auto receiver4 = Make(registry, keys, randomB, 99);
    Sockets::SocketSecuritySession rx4(receiver4, [](const uint8_t*, std::size_t) { return true; });
    bool failed = false;
    rx4.SetFailureCallback([&](const Security::SecurityResult& result) {
        failed = true;
        assert(result.Error == Security::SecurityError::ProtocolMismatch);
    });
    assert(rx4.Feed(tampered.data(), tampered.size()));
    assert(failed);

    std::cout << "Socket Security tests passed\n";
}
