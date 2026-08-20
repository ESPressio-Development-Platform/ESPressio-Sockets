#include <WiFi.h>
#include <ESPressio_Command.hpp>
#include <ESPressio_TCPCommandServer.hpp>

using namespace ESPressio;

constexpr char WiFiSSID[] = "YOUR_SSID";
constexpr char WiFiPassword[] = "YOUR_PASSWORD";

Sockets::TCPCommandServer CommandServer;

void setup() {
    Serial.begin(115200);

    WiFi.begin(WiFiSSID, WiFiPassword);
    while (WiFi.status() != WL_CONNECTED) {
        delay(100);
    }

    auto& commands =
        Command::CommandRegistry::GetInstance();

    commands.Command("system")
        .Command("status")
        .OnExecute(
            [](const Command::CommandContext&) {
                return Command::CommandResult::Ok(
                    "System OK"
                );
            }
        );

    auto& write =
        commands.Command("gpio")
            .Command("write");

    write.Parameter<int>("pin")
        .Range(0, 48);

    write.Parameter<bool>("state");

    write.OnExecute(
        [](const Command::CommandContext& context) {
            const int pin =
                context.Get<int>("pin");

            const bool state =
                context.Get<bool>("state");

            pinMode(pin, OUTPUT);
            digitalWrite(
                pin,
                state ? HIGH : LOW
            );

            return Command::CommandResult::Ok(
                "GPIO updated"
            );
        }
    );

    Sockets::TCPCommandServerConfig config;
    config.Port = 2323;
    config.MaximumClients = 4;
    config.Session.Mode =
        Sockets::SocketCommandMode::Line;
    config.Session.MaximumRequestBytes = 512;
    config.Session.DisconnectOnProtocolError = false;

    CommandServer.SetPolicy(
        [](const Sockets::SocketCommandInvocationContext& context) {
            Serial.printf(
                "Command session=%llu remote=%s:%u request=%llu raw=%s\n",
                static_cast<unsigned long long>(
                    context.Metadata.SessionID
                ),
                context.Metadata.RemoteAddress.c_str(),
                context.Metadata.RemotePort,
                static_cast<unsigned long long>(
                    context.Metadata.RequestID
                ),
                context.Invocation.raw.c_str()
            );

            return Command::CommandResult::Ok();
        }
    );

    if (!CommandServer.Initialize(config, commands)) {
        Serial.println(
            "TCP Command server failed to initialize"
        );
    } else {
        Serial.printf(
            "TCP Command server listening on port %u\n",
            config.Port
        );
    }
}

void loop() {
    delay(1000);
}
