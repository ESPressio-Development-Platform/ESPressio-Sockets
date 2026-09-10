# ESPressio Dependency Chart — Current Released Generation

![ESPressio Library Dependency Chart](ESPRESSIO_DEPENDENCY_CHART.svg)

## Released generation

```text
Observable
Serializable
Units
Timing
Threads
Event
Command
Security
Persistence
Sockets
ESP-Now
WiFi
Serial
```

## Sockets dependency position

```text
Sockets
    -> Observable main

Sockets optional integrations
    - - -> Event main
    - - -> Command main
    - - -> Security main
    - - -> Timing main
```

Observable remains the only required ESPressio dependency of core Sockets. Event, Command, Security and Timing integrations remain opt-in.

## Completed cascade

```text
Serializable
    -> Units
    -> Timing
    -> Threads
    -> Event
    -> Command / Security
    -> Persistence / Sockets / ESP-Now
    -> WiFi
    -> Serial
```

Event has no reverse dependency on Sockets. Serial remains terminal/downstream; ESPressio Tree remains standalone.
