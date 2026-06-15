# phosphor-chassis-power

## Overview

The `phosphor-chassis-power` application monitors GPIOs in multi-chassis
systems. It uses GPIOs to determine chassis presence and input power fault
status. It also enables chassis cleanup and re-integration functions.

## Application

`phosphor-chassis-power` is a single-threaded C++ executable. It is a daemon
process that runs continually. It is launched by systemd when the BMC reaches
the Ready state and before the system is powered on.

`phosphor-chassis-power` is driven by a system-specific JSON configuration file.
The config file is found and parsed when the app is started. The parsing process
creates a collection of C++ objects. These objects represent the chassis, GPIOs,
and fault checks to perform.

## JSON configuration file

`phosphor-chassis-power` is configured by a JSON configuration file. The
configuration file defines the GPIOs in the system and how they should be
monitored.

JSON configuration files are system-specific and are stored in the
[config_files](../config_files/) sub-directory.

[Documentation](config_file/README.md) is available on the configuration file
format.

If no configuration file is found for the current system, the system can still
be powered on/off, but `phosphor-chassis-power` would not monitor anything.

## D-Bus

### PowerSystemInputs

The existing
[PowerSystemInputs](https://github.com/openbmc/phosphor-dbus-interfaces/blob/master/yaml/xyz/openbmc_project/State/Decorator/PowerSystemInputs.interface.yaml)
D-Bus interface will be used on a new object path
`/xyz/openbmc_project/power/chassis/chassis{n}`:

- Name: `xyz.openbmc_project.State.Decorator.PowerSystemInputs`
- Description: Describes the status of power supplied to the chassis.
- Properties: Status (Good, Fault)
- Methods: None

The PowerSystemInputs will be for the input power fault read for the chassis
directly (i.e., NOT from the power supplies, as the power supplies will not be
able to communicate if they are not powered).

## Testing

Testing will be accomplished via automated and manual testing to verify
functionality.

### Automated Tests

- The [`test`](../test/) directory contains automated test cases using
  [GoogleTest](https://github.com/google/googletest) framework. The goal is to
  write automated test cases for as much application code as possible.
- CI tests that boot a system will indirectly test this application.

### Manual Testing

- Input power faults to a subset of the present chassis will be injected to
  ensure the system boots with the remaining chassis, logs the appropriate
  errors, and functions properly.
- Input power faults will be injected temporarily to ensure the chassis are
  configured and booted correctly.
- Missing chassis faults will be injected to ensure the system functions
  properly.
- Cable/hardware faults between the BMC and non-BMC-chassis will be injected to
  ensure the system functions properly.
- End-to-end boot testing will be performed to ensure that the correct hardware
  communication occurred, that the system boots, and that no errors occur.
