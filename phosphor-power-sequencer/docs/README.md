# phosphor-power-sequencer

## Overview

The `phosphor-power-sequencer` application powers all the chassis in the system
on and off. It also monitors the power good (pgood) state in each chassis.

## Application

`phosphor-power-sequencer` is a single-threaded C++ executable. It is a daemon
process that runs continually. It is launched by systemd when the BMC reaches
the Ready state and before the system is powered on.

`phosphor-power-sequencer` is driven by an optional, system-specific JSON
configuration file. The config file is found and parsed at runtime. The parsing
process creates a collection of C++ objects. These objects represent the
chassis, power sequencer devices, voltage rails, GPIOs, and fault checks to
perform.

See [Internal Design](internal_design.md) for more information.

## Power sequencer device

A power sequencer device enables (turns on) the voltage rails in the correct
order and monitors them for pgood faults.

`phosphor-power-sequencer` currently supports the following power sequencer
device types:

- UCD90160
- UCD90320

Additional device types can be supported by creating a new sub-class within the
PowerSequencerDevice class hierarchy. See [Internal Design](internal_design.md)
for more information.

If the power sequencer device type is not supported, `phosphor-power-sequencer`
can still power the system on/off and detect chassis pgood faults. However, it
will not be able to determine which voltage rail caused a pgood fault.

## Powering on the system

`phosphor-power-sequencer` uses the power sequencer device to power on all main
(non-standby) voltage rails in each chassis.

See [Powering On](powering_on.md) for more information.

## Powering off the system

`phosphor-power-sequencer` uses the power sequencer device to power off all main
(non-standby) voltage rails in each chassis.

See [Powering Off](powering_off.md) for more information.

## Monitoring chassis pgood

`phosphor-power-sequencer` periodically reads the chassis pgood state from the
power sequencer device. See
[Monitoring Chassis Power Good](monitoring_chassis_pgood.md) for more
information.

## Chassis pgood faults

If the chassis pgood state is false when it should be true, a chassis pgood
fault has occurred. `phosphor-power-sequencer` uses information from the power
sequencer device to determine the cause.

See [Power Good Faults](pgood_faults.md) for more information.

## JSON configuration file

`phosphor-power-sequencer` is configured by an optional JSON configuration file.
The configuration file defines the voltage rails in the system and how they
should be monitored.

JSON configuration files are system-specific and are stored in the
[config_files](../config_files/) sub-directory.

[Documentation](config_file/README.md) is available on the configuration file
format.

If no configuration file is found for the current system,
`phosphor-power-sequencer` can still power the system on/off and detect chassis
pgood faults. However, it will not be able to determine which voltage rail
caused a pgood fault.

## Testing

Automated test cases exist for most of the code in this application. See
[Testing](testing.md) for more information.
