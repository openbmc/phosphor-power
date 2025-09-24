# Multiple Chassis

## Overview

A BMC-based system can contain one or more chassis. A chassis is typically a
physical enclosure that contains system components such as CPUs, fans, power
supplies, and PCIe cards.

A chassis can be stand-alone, such as a tower or desktop. A chassis can also be
designed to be mounted in an equipment rack.

For the `phosphor-regulators` application, the term "single chassis system"
means the system type has a maximum configuration of one chassis. If the system
type has a maximum configuration of multiple chassis, then it is considered a
"multiple chassis system" even if the current system only contains one chassis.

`phosphor-regulators` only supports powering on and off the entire system. It
does not support powering on/off individual chassis independent of the rest of
the system.

## Defining the chassis in a system

The [JSON configuration file](config_file/README.md) contains an array of one or
more [chassis](config_file/chassis.md) objects. Each chassis object corresponds
to a physical chassis in the system.

## Differences between single and multiple chassis systems

### System and chassis power state

In a single chassis system, the system and chassis power state are identical.
Both are either on or off.

In a multiple chassis system, each chassis has its own power state. Even if the
system is on, an individual chassis may be off. This can occur if that chassis
does not have input power or has a critical hardware problem.

See [Chassis Status](../phosphor-power-sequencer/docs/chassis_status.md) for
more information.

### Configuration

In a single chassis system, regulator configuration is always performed on the
chassis.

In a multiple chassis system, regulator configuration will only be performed on
chassis with the proper status. For example, a chassis that is missing or has no
input power cannot be configured.

See [Regulator Configuration](configuration.md) for more information.

### Monitoring

In a single chassis system, regulator monitoring is always performed on the
chassis.

In a multiple chassis system, regulator monitoring will only be performed on
chassis with the proper status. For example, a chassis that is missing or has no
input power cannot be monitored.

See [Regulator Monitoring](monitoring.md) for more information.
