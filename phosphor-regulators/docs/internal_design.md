# Internal Design

## Key classes

- Manager
  - Top level class created in `main()`.
  - Loads the JSON configuration file.
  - Implements the D-Bus `configure` and `monitor` methods.
  - Contains a System object.
- System
  - Represents the computer system being controlled and monitored by the BMC.
  - Contains one or more Chassis objects.
- Chassis
  - Represents an enclosure that can be independently powered off and on by the
    BMC.
  - Small and mid-sized systems may contain a single Chassis.
  - In a large rack-mounted system, each drawer may correspond to a Chassis.
  - Contains one or more Device objects.
- Device
  - Represents a hardware device, such as a voltage regulator or I/O expander.
  - Contains zero or more Rail objects.
- Rail
  - Represents a voltage rail produced by a voltage regulator, such as 1.1V.
- Services
  - Abstract base class that provides access to a collection of system services
    like error logging, journal, vpd, and hardware presence.
  - The BMCServices child class provides the real implementation.
  - The MockServices child class provides a mock implementation that can be used
    in gtest test cases.
