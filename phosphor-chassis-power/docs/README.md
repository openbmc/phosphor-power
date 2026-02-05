# phosphor-chassis-power

## Overview

The `phosphor-chassis-power` application monitors the GPIO to activate and
deactivate a chassis.

## Application

`phosphor-chassis-power` is a single-threaded C++ executable. It is a daemon
process that runs continually. It is launched by systemd when the BMC reaches
the Ready state and before the system is powered on.

`phosphor-chassis-power` is driven by a system-specific JSON configuration file.
The config file is found and parsed at runtime. The parsing process creates a
collection of C++ objects. These objects represent the chassis, GPIOs, and fault
checks to perform.

## JSON configuration file

`phosphor-chassis-power` is configured by a JSON configuration file. The
configuration file defines the GPIOs in the system and how they should be
monitored.

JSON configuration files are system-specific and are stored in the
[config_files](../config_files/) sub-directory.

[Documentation](config_file/README.md) is available on the configuration file
format.

If no configuration file is found for the current system,
`phosphor-chassis-power` can still power the system on/off, but would not
monitor anything. NOTE:TBD
