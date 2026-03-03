# Internal Design

## Key classes

- PowerInterface
  - Abstract base class that defines the `org.openbmc.control.Power` D-Bus
    interface.
  - The `state` property is set to power the system/chassis on or off. This
    contains the desired power state.
  - The `pgood` property contains the actual power state of the system/chassis.
- SystemPowerInterface
  - Sub-class of PowerInterface that handles system level D-Bus methods and
    properties.
- ChassisPowerInterface
  - Sub-class of PowerInterface that handles chassis level D-Bus methods and
    properties.
- Manager
  - Created in `main()`. Handles the event loop.
  - Finds and loads the JSON configuration file for the system type. This
    creates an instance of the System class.
  - Provides a timer that monitors the system and chassis every second by
    calling the System::monitor() method.
- System
  - The computer system being controlled and monitored by the BMC.
- Chassis
  - A chassis within the system. Chassis are typically a physical enclosure that
    contains system components such as CPUs, fans, power supplies, and PCIe
    cards.
- Rail
  - A voltage rail that is enabled or monitored by the power sequencer device.
- PowerSequencerDevice
  - Abstract base class for a power sequencer device.
  - Defines virtual methods that must be implemented by all child classes.
- BasicDevice
  - Sub-class of PowerSequencerDevice that provides basic functionality.
  - Implements power sequencer properties from the JSON configuration file.
- GPIOsOnlyDevice
  - Sub-class of BasicDevice that only uses the named GPIOs and does not
    otherwise communicate with the device.
- StandardDevice
  - Sub-class of BasicDevice that implements the standard pgood fault detection
    algorithm.
- PMBusDriverDevice
  - Sub-class of StandardDevice for power sequencer devices that are bound to a
    PMBus device driver.
- UCD90xDevice
  - Sub-class of PMBusDriverDevice for the UCD90X family of power sequencer
    devices.
- UCD90160Device
  - Sub-class of UCD90xDevice representing a UCD90160 power sequencer device.
- UCD90320Device
  - Sub-class of UCD90xDevice representing a UCD90320 power sequencer device.
- Services
  - Abstract base class that provides an interface to system services like error
    logging and the journal.
- BMCServices
  - Sub-class of Services with real implementation of methods.
- MockServices
  - Sub-class of Services with mock implementation of methods for automated
    testing.
