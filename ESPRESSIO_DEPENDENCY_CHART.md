# ESPressio Dependency Chart — Current Cascade Generation

![ESPressio Library Dependency Chart](ESPRESSIO_DEPENDENCY_CHART.svg)

This document records the dependency generation validated by ESPressio Sockets 0.7.3. Arrows point from the consuming library to the library it consumes.

- **Required** — the dependency is part of the library's normal/core contract.
- **Opt-in** — the dependency is introduced only when the corresponding integration/header is selected.

## Cascade generation

```text
Observable    3.0.2
Serializable  0.11.3
Units         0.2.7
Timing        2.2.8
Threads       3.1.7
Event         6.0.3
Command       1.0.3
Security      0.4.2
Persistence   0.3.2
Sockets       0.7.3
```

## Required dependencies

```text
Observable 3.0.2
    -> none

Serializable 0.11.3
    -> none

Units 0.2.7
    -> none

Timing 2.2.8
    -> Units >= 0.2.7 < 1.0.0
    -> Observable >= 3.0.2 < 4.0.0

Threads 3.1.7
    -> Timing >= 2.2.8 < 3.0.0
    -> Observable >= 3.0.2 < 4.0.0

Event 6.0.3
    -> Threads >= 3.1.7 < 4.0.0
    -> Timing >= 2.2.8 < 3.0.0
    -> Observable >= 3.0.2 < 4.0.0

Command 1.0.3
    -> Observable >= 3.0.2 < 4.0.0

Security 0.4.2
    -> Observable >= 3.0.2 < 4.0.0

Persistence 0.3.2
    -> none

Sockets 0.7.3
    -> Observable >= 3.0.2 < 4.0.0
```

## Opt-in integrations

```text
Units
    - - -> Serializable >= 0.11.3 < 1.0.0
            Serializable Unit variants

Event
    - - -> Serializable >= 0.11.3 < 1.0.0
            Serializable Events / Event Transport

Command
    - - -> Event >= 6.0.3 < 7.0.0
            Command-owned Event types / CommandRegistryEventBridge

Security
    - - -> Event >= 6.0.3 < 7.0.0
            Security-owned Event types / TransportSecurityEventBridge

Persistence
    - - -> Serializable >= 0.11.3 < 1.0.0
            typed/protected persistence; Security reached through Serializable protection

Sockets
    - - -> Event >= 6.0.3 < 7.0.0
    - - -> Command >= 1.0.3 < 2.0.0
    - - -> Security >= 0.4.2 < 1.0.0
    - - -> Timing >= 2.2.8 < 3.0.0
```

`JsonCommandInterpreter` optionally consumes external **ArduinoJson 7.x**. ArduinoJson is not an ESPressio library and is therefore not represented as an ESPressio graph edge.

## Dependency-direction invariants

Event owns the generic Event mechanism. Domain-specific Event types and bridges belong to the lowest-order library that owns the represented concept without introducing a reverse dependency:

```text
Command  - - -> Event
Security - - -> Event
Sockets  - - -> Event

Event -> Command   NONE
Event -> Security  NONE
Event -> Sockets   NONE
```

Timing and Threads Event bridges remain in Event because Event already requires Timing and Threads for its own responsibilities; moving those bridges upstream would create reverse dependencies.

Serial remains terminal/downstream. No upstream ESPressio library should depend on Serial.

ESP-Now is the next parallel transport cascade target and is intentionally not presented here as already advanced until its release is prepared.
