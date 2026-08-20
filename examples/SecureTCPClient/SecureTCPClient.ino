#include <Arduino.h>
#include <WiFi.h>

#include <ESPressio_Security.hpp>
#include <ESPressio_SocketSecuritySession.hpp>

using namespace ESPressio;

const char* WifiSsid = "YOUR_WIFI_SSID";
const char* WifiPassword = "YOUR_WIFI_PASSWORD";
const char* ServerHost = "192.168.1.10";
const uint16_t ServerPort = 2323;
static constexpr uint8_t DemoProtocol = 70;

WiFiClient client;
Security::AES256GCMCipher cipher;
Security::AeadCipherRegistry ciphers;
Security::StaticKeyProvider keys;
Security::ESP32RandomSource randomSource;
Security::TransportSecurity* security = nullptr;
Sockets::SocketSecuritySession* secureSession = nullptr;

void setup() {
    Serial.begin(115200);
    WiFi.begin(WifiSsid, WifiPassword);
    while (WiFi.status() != WL_CONNECTED) delay(100);

    ciphers.Register(cipher);
    const uint8_t demoKey[32] = {
        0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,
        0x08,0x09,0x0A,0x0B,0x0C,0x0D,0x0E,0x0F,
        0x10,0x11,0x12,0x13,0x14,0x15,0x16,0x17,
        0x18,0x19,0x1A,0x1B,0x1C,0x1D,0x1E,0x1F
    };
    keys.Add(1, Security::AeadAlgorithm::AES256GCM, demoKey, sizeof(demoKey));

    Security::TransportSecurityConfig config;
    config.Policy = Security::TransportSecurityPolicy::Required;
    config.OutboundAlgorithm = Security::AeadAlgorithm::AES256GCM;
    config.OutboundKeyID = 1;
    config.SenderID = ESP.getEfuseMac();

    static Security::TransportSecurity securityInstance(ciphers, keys, randomSource, config);
    security = &securityInstance;

    if (!client.connect(ServerHost, ServerPort)) {
        Serial.println("TCP connection failed");
        return;
    }

    static Sockets::SocketSecuritySession session(
        *security,
        [](const uint8_t* data, std::size_t size) {
            return client.connected() && client.write(data, size) == size;
        }
    );
    secureSession = &session;

    secureSession->SetReceiveCallback([](const Security::UnprotectedPayload& opened) {
        Serial.printf("secure RX protocol=%u sender=%llu session=%llu sequence=%llu bytes=%u\n",
            opened.Protocol,
            static_cast<unsigned long long>(opened.SenderID),
            static_cast<unsigned long long>(opened.SessionID),
            static_cast<unsigned long long>(opened.Sequence),
            static_cast<unsigned>(opened.Data.size()));
    });

    secureSession->SetFailureCallback([](const Security::SecurityResult& result) {
        Serial.printf("secure frame rejected error=%u\n", static_cast<unsigned>(result.Error));
    });

    const char message[] = "authenticated socket payload";
    Security::SecurityResult result;
    const bool sent = secureSession->Send(DemoProtocol, message, sizeof(message)-1, &result);
    Serial.printf("secure send=%s protected=%s\n", sent ? "OK" : "FAILED", result.Protected ? "yes" : "no");
}

void loop() {
    if (secureSession == nullptr || !client.connected()) {
        delay(100);
        return;
    }

    uint8_t buffer[256];
    const int available = client.available();
    if (available > 0) {
        const int count = client.read(buffer, min(available, static_cast<int>(sizeof(buffer))));
        if (count > 0) secureSession->Feed(buffer, static_cast<std::size_t>(count));
    }
    delay(1);
}
