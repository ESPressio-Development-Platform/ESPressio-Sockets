#include <cassert>
#include <cstdint>
#include <vector>

#include <ESPressio_SocketSecuritySession.hpp>

using namespace ESPressio;

class SessionObserver final : public Sockets::ISocketSecuritySessionObserver {
public:
    int Faulted = 0;
    int Reset = 0;

    void OnSocketSecuritySessionFaulted(const Security::SecurityResult&) override {
        ++Faulted;
    }

    void OnSocketSecuritySessionReset() override {
        ++Reset;
    }
};

int main() {
    Security::AeadCipherRegistry ciphers;
    Security::StaticKeyProvider keys;
    Security::StandardRandomSource random;
    Security::TransportSecurityConfig config;
    config.Policy = Security::TransportSecurityPolicy::Disabled;
    Security::TransportSecurity security(ciphers, keys, random, config);

    Sockets::SocketSecuritySession session(
        security,
        [](const uint8_t*, std::size_t) { return true; }
    );

    SessionObserver observer;
    auto handle = session.RegisterObserver(&observer);
    assert(handle);

    const uint8_t malformedLength[5] = {0, 0, 0, 0, 1};
    assert(!session.Feed(malformedLength, sizeof(malformedLength)));
    assert(observer.Faulted == 1);

    session.Reset();
    assert(observer.Reset == 1);

    handle.reset();
    session.Reset();
    assert(observer.Reset == 1);
    return 0;
}
