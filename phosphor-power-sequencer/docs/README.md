# phosphor-power-sequencer

## Overview

The phosphor-power-sequencer application powers the chassis on/off and monitors
the power sequencer device.

If the chassis power good (pgood) status changes to false unexpectedly, the
application uses information from the power sequencer device to determine the
cause. Typically this is a voltage rail that shut down due to a fault.

## Application

The application is a single-threaded C++ executable. It is a 'daemon' process
that runs continually. The application is launched by systemd when the BMC
reaches the Ready state and before the chassis is powered on.

The application is driven by an optional, system-specific JSON configuration
file. The config file is found and parsed at runtime. The parsing process
creates a collection of C++ objects. These objects represent the power sequencer
device, voltage rails, GPIOs, and fault checks to perform.

## Power sequencer device

A power sequencer device enables (turns on) the voltage rails in the correct
order and monitors them for pgood faults.

This application currently supports the following power sequencer device types:

- UCD90160
- UCD90320

Additional device types can be supported by creating a new sub-class within the
PowerSequencerDevice class hierarchy.

## Powering on the chassis

- A BMC application or script sets the `state` property to 1 on the
  `org.openbmc.control.Power` D-Bus interface.
- The phosphor-power-sequencer application writes the value 1 to the named GPIO
  `power-chassis-control`.
  - This GPIO is defined in the device tree. The GPIO name is defined in the
    [Device Tree GPIO Naming in OpenBMC document](https://github.com/openbmc/docs/blob/master/designs/device-tree-gpio-naming.md)
- The power sequencer device powers on all the voltage rails in the correct
  order.
- When all rails have been successfully powered on, the power sequencer device
  sets the named `power-chassis-good` GPIO to the value 1.
  - This GPIO is defined in the device tree. The GPIO name is defined in the
    [Device Tree GPIO Naming in OpenBMC document](https://github.com/openbmc/docs/blob/master/designs/device-tree-gpio-naming.md)
- The phosphor-power-sequencer application sets the `pgood` property to 1 on the
  `org.openbmc.control.Power` D-Bus interface.
- The rest of the boot continues

## Powering off the chassis

- A BMC application or script sets the `state` property to 0 on the
  `org.openbmc.control.Power` D-Bus interface.
- The phosphor-power-sequencer application writes the value 0 to the named GPIO
  `power-chassis-control`.
- The power sequencer device powers off all the voltage rails in the correct
  order.
- When all rails have been successfully powered off, the power sequencer device
  sets the named `power-chassis-good` GPIO to the value 0.
- The phosphor-power-sequencer application sets the `pgood` property to 0 on the
  `org.openbmc.control.Power` D-Bus interface.

## Pgood fault

A power good (pgood) fault occurs in two scenarios:

- When attempting to power on the chassis:
  - The power sequencer device is powering on all voltage rails in order, and
    one of the rails does not turn on.
- After the chassis was successfully powered on:
  - A voltage rail suddenly turns off or stops providing the expected level of
    voltage. This could occur if the voltage regulator stops working or if it
    shuts itself off due to exceeding a temperature/voltage/current limit.

If the pgood fault occurs when attempting to power on the chassis, the chassis
pgood signal never changes to true.

If the pgood fault occurs after the chassis was successfully powered on, the
chassis pgood signal changes from true to false. This application monitors the
chassis power good status by reading the named GPIO `power-chassis-good`.

## Identifying the cause of a pgood fault

It is very helpful to identify which voltage rail caused the pgood fault. That
determines what hardware potentially needs to be replaced.

Determining the correct rail requires the following:

- The power sequencer device type is supported by this application
- A JSON config file is defined for the current system

If those requirements are met, the application will attempt to determine which
voltage rail caused the chassis pgood fault. If found, an error is logged
against that specific rail.

If those requirements are not met, a general pgood fault error is logged.

## JSON configuration file

This application is configured by an optional JSON configuration file. The
configuration file defines the voltage rails in the system and how they should
be monitored.

JSON configuration files are system-specific and are stored in the
[config_files](../config_files/) sub-directory.

[Documentation](config_file/README.md) is available on the configuration file
format.

If no configuration file is found for the current system, this application can
still power the chassis on/off and detect chassis pgood faults. However, it will
not be able to determine which voltage rail caused a pgood fault.

## Key Classes

- PowerInterface
  - Defines the `org.openbmc.control.Power` D-Bus interface.
  - The `state` property is set to power the chassis on or off. This contains
    the desired power state.
  - The `pgood` property contains the actual power state of the chassis.
- PowerControl
  - Created in `main()`. Handles the event loop.
  - Sub-class of PowerInterface that provides a concrete implementation of the
    `org.openbmc.control.Power` D-Bus interface.
  - Finds and loads the JSON configuration file.
  - Finds power sequencer device information.
  - Creates a sub-class of PowerSequencerDevice that matches power sequencer
    device information.
  - Powers the chassis on and off using the `power-chassis-control` named GPIO.
  - Monitors the chassis pgood status every 3 seconds using the
    `power-chassis-good` named GPIO.
  - Enforces a minimum power off time of 15 seconds from cold start and 25
    seconds from power off.
- DeviceFinder
  - Finds power sequencer device information on D-Bus published by
    EntityManager.
- Rail
  - A voltage rail that is enabled or monitored by the power sequencer device.
- PowerSequencerDevice
  - Abstract base class for a power sequencer device.
  - Defines virtual methods that must be implemented by all child classes.
- StandardDevice
  - Sub-class of PowerSequencerDevice that implements the standard pgood fault
    detection algorithm.
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

## Testing

Automated test cases exist for most of the code in this application. See
[testing.md](testing.md) for more information.
