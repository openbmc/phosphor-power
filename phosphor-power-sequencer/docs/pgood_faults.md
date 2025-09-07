# Power Good Faults

## Overview

The power sequencer device provides a chassis power good (pgood) signal. This
indicates that all of the main (non-standby) voltage rails are powered on.

If the chassis pgood state is false when it should be true, a chassis power good
(pgood) fault has occurred.

## Pgood fault while powering on the system

When the power sequencer device is powering on the main voltage rails in order,
one of the rails may fail to power on. This is often due to a hardware problem.

When a voltage rail fails to power on, the power sequencer device may
immediately indicate an error. However, the device may instead wait indefinitely
for the rail to power on. In both cases the chassis pgood signal never changes
to true.

## Pgood fault after the system was powered on

A pgood fault can occur after a system has been powered on. The system may have
been successfully running for days or months.

A voltage rail may suddenly power off or stop providing the expected level of
voltage. This could occur if the voltage regulator stops working or if it shuts
itself off due to exceeding a temperature/voltage/current limit.

The power sequencer device will detect that the voltage rail has failed. The
device will change the state of the chassis pgood signal to false. The device
may also power off several other related voltage rails, depending on how the
hardware is configured.

## Pgood fault handling

`phosphor-power-sequencer` detects a pgood fault by monitoring the chassis pgood
signal:

- Powering on chassis: pgood signal never changes to true.
- Chassis was powered on: pgood signal changes from true to false.

When a pgood fault is detected, `phosphor-power-sequencer` will perform the
following steps:

- Use information from the power sequencer device to determine the cause of the
  fault.
- Log an error with information about the fault.
- If this is a single chassis system:
  - The system will be [powered off](powering_off.md).
- If this is a multiple chassis system:
  - Wait a short period of time, and then check if all the other chassis that
    were powered on are also experiencing a pgood fault. If so, check if any
    chassis is experiencing a brownout or blackout. This determines whether this
    is a chassis-specific problem or a system-wide problem due to a
    [Power Loss](power_loss.md).
  - If this is a chassis-specific problem, add the inventory path of the chassis
    to the error log. This may result in hardware isolation, which will cause
    the `Enabled` property of the chassis to be false.
  - The system will be powered [off](powering_off.md) and then
    [on](powering_on.md) again.
  - Chassis with an `Enabled` value of false will **not** be powered back on.

See [Chassis Status](chassis_status.md) for more information on the `Enabled`
property.

Note that when a pgood error happens **during** a power on attempt, the
`phosphor-chassis-state-manager` application handles the power off/power cycle.
When the pgood error happens **after** the system was powered on, the
`phosphor-power-sequencer` application handles the power off/power cycle. This
is due to the complex service file relationships that occur during a power on
attempt.

## Determining the cause of a pgood fault

It is very helpful to determine which voltage rail caused a pgood fault. That
determines what hardware potentially needs to be replaced.

Determining the correct rail requires the following:

- The power sequencer device type is supported by `phosphor-power-sequencer`.
- A [JSON configuration file](config_file/README.md) is defined for the system.

If those requirements are not met, a general pgood error will be logged.

If those requirements are met, `phosphor-power-sequencer` will attempt to
determine which voltage rail caused the chassis pgood fault. The following
methods are supported in the JSON configuration file:

- Read a GPIO from the power sequencer device
- Check the PMBus STATUS_VOUT command value
- Compare the PMBus READ_VOUT value to the PMBus VOUT_UV_FAULT_LIMIT value

Multiple methods might need to be used on the same rail. For example, the PMBus
STATUS_VOUT error bits might be set for a pgood fault after the system powered
on, but they might not be set during a power on attempt because the power
sequencer is waiting indefinitely for the rail to power on.

See the [rail](config_file/rail.md) object in the configuration file for more
information.

If a specific voltage rail is found, an error is logged against that rail.

If the voltage rail is from the power supplies, and the `phosphor-power-supply`
application found a power supply error, then the power supply error is logged as
the cause of the pgood fault.

If no voltage rail is found, a general pgood error is logged.
