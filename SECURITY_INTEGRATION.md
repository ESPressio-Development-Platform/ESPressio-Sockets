# ESPressio Sockets Security Integration

ESPressio Sockets 0.4.0 adds optional ESPressio Security 0.1.x integration for both stream-oriented and datagram-oriented socket mechanisms.

## Dependency

Core ESPressio Sockets remains independent of Security. Applications selecting the secure facilities must provide:

```text
ESPressio Security >= 0.1.0 < 1.0.0
```

PlatformIO:

```ini
lib_deps =
    https://github.com/Flowduino/ESPressio-Sockets@^0.4.0
    https://github.com/Flowduino/ESPressio-Security@^0.1.0
```

## Architecture

```text
Event / Command / application protocol
              |
              v
      ESPressio Security
              |
       +------+------+
       |             |
       v             v
SocketSecurity   SocketSecurity
Session          Datagram
       |             |
       v             v
TCP/TLS/WS       UDP/message socket
```

Neither adapter implements cryptography. Both delegate encryption, authentication, sender/session identity, key selection, and replay protection to `Security::TransportSecurity`.

## Stream Sockets

TCP/TLS and other byte streams do not preserve application message boundaries. `SocketSecuritySession` therefore wraps each protected Security envelope in a four-byte little-endian length prefix.

The receiver accepts arbitrary byte chunks and reconstructs complete secure frames before authentication/decryption.

```cpp
Sockets::SocketSecuritySession secure(
    security,
    [&](const uint8_t* data, std::size_t size) {
        return client.write(data, size) == size;
    }
);

secure.SetReceiveCallback(
    [](const Security::UnprotectedPayload& opened) {
        // authenticated plaintext
    }
);
```

Feed data received from the underlying stream:

```cpp
secure.Feed(buffer, receivedBytes);
```

The parser supports:

- frames split across multiple reads;
- multiple complete frames in one read;
- bounded declared frame lengths;
- malformed/oversized frame rejection;
- explicit `Reset()` after a protocol-level framing failure.

## Datagram Sockets

UDP and other message-oriented carriers already preserve packet boundaries. `SocketSecurityDatagram` therefore carries one complete ESPressio Security envelope per datagram with no additional stream framing.

```cpp
Sockets::SocketSecurityDatagram secure(
    security,
    [&](const uint8_t* data, std::size_t size) {
        // send one datagram
        return SendDatagram(data, size);
    }
);
```

On receive:

```cpp
secure.Receive(datagram, datagramLength);
```

Replay of an otherwise-valid datagram is rejected by Security's sender/key/session/sequence replay window.

## Security Policies

The socket adapters preserve Security's explicit policies:

```text
Disabled
Preferred
Required
```

For network-facing Commands or control traffic, `Required` is normally the appropriate mode.

## TLS and ESPressio Security

ESPressio Security does not replace TLS automatically, nor does TLS make this layer redundant in every architecture.

They protect different boundaries:

```text
TLS
    connection/session transport protection

ESPressio Security
    application/transport payload identity, protocol binding,
    key IDs, authenticated session epochs, replay protection
```

Applications may use Security over a plaintext TCP/UDP mechanism or combine it with TLS as defense-in-depth/end-to-end application protection.

## Command Integration

The structured bytes produced by `SocketCommandProtocol` can be carried through `SocketSecuritySession` instead of directly over a raw stream. This keeps Command unaware of cryptography while ensuring a remote request is authenticated/decrypted before it reaches `CommandRegistry`.

The same principle applies to Event payloads and future socket-carried protocols.

## Failure Handling

`SetFailureCallback` receives `SecurityResult` for authentication, replay, key, algorithm, protocol, or frame-limit failures. Key material is never exposed through these diagnostics.

## Example

See:

```text
examples/SecureTCPClient/SecureTCPClient.ino
```

The example demonstrates adapting a `WiFiClient` read/write stream to `SocketSecuritySession`. Its embedded key is demonstration-only; production keys must be provisioned securely.
