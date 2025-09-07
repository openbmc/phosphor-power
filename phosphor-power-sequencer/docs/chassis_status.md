# Chassis Status

## Overview

There are multiple D-Bus interfaces and properties that describe the chassis
status. The `phosphor-power-sequencer` application publishes one of those
interfaces. The other interfaces are published by different applications and are
used as input by `phosphor-power-sequencer.`

## state and pgood properties

The `state` and `pgood` properties exist on the `org.openbmc.control.Power`
D-Bus interface.

`state` has a value of 0 or 1. `state` represents the desired power state for
the chassis. 0 means off, and 1 means on. This property is set when the system
is being powered [on](powering_on.md) or [off](powering_off.md).

`pgood` has a value of 0 or 1. `pgood` represents the power good (pgood) state
of the chassis. 0 means off, and 1 means on. This is the actual, current power
state. This property is set based on the chassis power good signal from the
power sequencer device.

`phosphor-power-sequencer` publishes the `org.openbmc.control.Power` interface
on the following object paths:

- `/org/openbmc/control/power0`: Represents the entire system
- `/org/openbmc/control/power1`: Represents chassis 1
- `/org/openbmc/control/power2`: Represents chassis 2
- ...
- `/org/openbmc/control/powerN`: Represents chassis N

### Single chassis system

On a single chassis system, only the first two object paths are published,
representing the entire system and chassis 1. The values of the `state` and
`pgood` properties are identical for both object paths.

When the system is powered off, the `state` and `pgood` properties for both
object paths are 0.

When the system is being powered on, the `state` property of both object paths
is set to 1. When the chassis has successfully powered on, the `pgood` property
of both object paths is set to 1.

When the system is being powered off, the `state` property of both object paths
is set to 0. When the chassis has successfully powered off, the `pgood` property
of both object paths is set to 0.

### Multiple chassis system

On a multiple chassis system with N chassis, all of the object paths above are
published. The first object path represents the state of the entire system, and
the other object paths represent the state of the individual chassis.

When the system is powered off, the `state` and `pgood` properties for all of
the object paths are 0.

When the system is being powered on, the `state` will be changed to 1 for the
system and for all chassis that **can** be powered on. It may not be possible to
power on some chassis, such as if they are missing or have no input power. See
[Powering On](powering_on.md) for more information.

When an individual chassis has successfully powered on, the `pgood` property for
that object path will change to 1. When all chassis whose `state` was set to 1
have successfully powered on, the `pgood` property for the system object path
will change to 1.

If a power good fault occurs in a chassis after the system powered on, the
`pgood` property for that chassis's object path will change to 0. The `pgood`
property for the system object path will remain set to 1 for a period of time.
Eventually the system will be powered off due to the power good fault. See
[Power Good Faults](pgood_faults.md) for more information.

When the system is being powered off, the `state` will be changed to 0 for the
system and for all chassis that are being powered off.

When an individual chassis has successfully powered off, the `pgood` property
for that object path will change to 0. When all chassis have successfully
powered off, the `pgood` property for the system object path will change to 0.

### Initial property values

When `phosphor-power-sequencer` is started, it needs to set initial values for
the `state` and `pgood` properties. This occurs when a system first receives
input power or after the BMC is reset.

The chassis power good signal is read from the power sequencer device. Both the
`state` and `pgood` properties are set to the value of the power good signal.
For example, if the chassis power good is true, both `state` and `pgood` are set
to the value 1. Thus, `phosphor-power-sequencer` assumes the chassis `state`
based on the chassis `pgood`.

In a multiple chassis system, it might not be possible to read the chassis power
good signal. For example, the chassis might not be present or might have no
input power. See the sections below for information about how the `pgood`
property will be set.

## Present property

The `Present` property exists in the [`xyz.openbmc_project.Inventory.Item`][1]
D-Bus interface.

This interface is published on the D-Bus inventory path for each chassis.
`phosphor-power-sequencer` does not publish this interface, but it checks the
`Present` property on multiple chassis systems.

The `Present` property indicates whether a chassis is physically present.

If `Present` is set to false for a chassis:

- The chassis power good signal from the power sequencer will **not** be read.
  Since the chassis is not present, the `pgood` property on the
  `org.openbmc.control.Power` interface for the chassis will be set to 0.
- The chassis will **not** be powered on when the system is being powered on.
  The `state` property on the `org.openbmc.control.Power` interface for the
  chassis will remain set to 0.

## Available property

The `Available` property exists in the
[`xyz.openbmc_project.State.Decorator.Availability`][2] D-Bus interface.

This interface is published on the D-Bus inventory path for each chassis.
`phosphor-power-sequencer` does not publish this interface, but it checks the
`Available` property on multiple chassis systems.

This interface is optional. If the interface exists and `Available` is set to
false, it means that communication to the chassis is not possible. For example,
the chassis does not have any input power ([blackout](power_loss.md)) or
communication cables to the BMC are disconnected.

If `Available` is set to false for a chassis:

- The chassis power good signal from the power sequencer will **not** be read.
  Since communication to the chassis is not possible, the value of the power
  good signal is unknown. The following algorithm is used to set the value:
  - If the chassis is experiencing a blackout, the chassis has no input power
    and `pgood` will be set to 0.
  - If all other chassis have a `pgood` value of 0, the `pgood` value will be
    set to 0 for this chassis.
  - If at least one other chassis has a `pgood` value of 1, the `pgood` value
    will be set to 1 for this chassis.
- The chassis will **not** be powered on when the system is being powered on.
  The `state` property on the `org.openbmc.control.Power` interface for the
  chassis will remain set to 0.

## Enabled property

The `Enabled` property exists in the [`xyz.openbmc_project.Object.Enable`][3]
D-Bus interface.

This interface is published on the D-Bus inventory path for each chassis.
`phosphor-power-sequencer` does not publish this interface, but it checks the
`Enabled` property on multiple chassis systems.

This interface is optional. If the interface exists and `Enabled` is set to
false, it means that the chassis has been put in hardware isolation (guarded). A
critical error has been detected in the chassis, and it will not be used when
the system is powered on.

If `Enabled` is set to false for a chassis:

- The chassis power good signal from the power sequencer will be read. The
  `pgood` property on the `org.openbmc.control.Power` interface for the chassis
  will be set to the value of this signal.
- The chassis will **not** be powered on when the system is being powered on.
  The `state` property on the `org.openbmc.control.Power` interface for the
  chassis will remain set to 0.

## Status property

The `Status` property exists in the
[`xyz.openbmc_project.State.Decorator.PowerSystemInputs`][4] D-Bus interface.

`phosphor-power-sequencer` does not publish this interface, but it checks the
`Status` property on multiple chassis systems.

This interface is optional. If the interface exists and `Status` is set to
`Fault`, it means that the chassis is experiencing a blackout or brownout. A
[power loss](power_loss.md) is occurring, such as a power company utility
failure or an unplugged power cord. See the sub-sections below for more
information.

### Chassis power status

The `phosphor-chassis-power` application publishes the
`xyz.openbmc_project.State.Decorator.PowerSystemInputs` interface on the
following object paths:

- `/xyz/openbmc_project/power/chassis/chassis1`
- `/xyz/openbmc_project/power/chassis/chassis2`
- ...
- `/xyz/openbmc_project/power/chassis/chassisN`

If `Status` is set to `Fault` for a chassis:

- The chassis is experiencing a blackout.
- The chassis power good signal from the power sequencer will **not** be read.
  Since the chassis has no input power, the `pgood` property on the
  `org.openbmc.control.Power` interface for the chassis will be set to 0.
- The chassis will **not** be powered on when the system is being powered on.
  The `state` property on the `org.openbmc.control.Power` interface for the
  chassis will remain set to 0. An error will be logged indicating that the
  chassis was not powered on due to an input power problem.

### Power supplies power status

The `phosphor-power-supply` application publishes the
`xyz.openbmc_project.State.Decorator.PowerSystemInputs` interface on the
following object paths:

- `/xyz/openbmc_project/power/power_supplies/chassis1/psus`
- `/xyz/openbmc_project/power/power_supplies/chassis2/psus`
- ...
- `/xyz/openbmc_project/power/power_supplies/chassisN/psus`

If `Status` is set to `Fault` for the PSUs in a chassis:

- The chassis is experiencing a brownout.
- The chassis power good signal from the power sequencer will be read. The
  `pgood` property on the `org.openbmc.control.Power` interface for the chassis
  will be set to the value of this signal.
- The chassis will **not** be powered on when the system is being powered on.
  The `state` property on the `org.openbmc.control.Power` interface for the
  chassis will remain set to 0. An error will be logged indicating that the
  chassis was not powered on due to an input power problem.

[1]:
  https://github.com/openbmc/phosphor-dbus-interfaces/blob/master/yaml/xyz/openbmc_project/Inventory/Item.interface.yaml
[2]:
  https://github.com/openbmc/phosphor-dbus-interfaces/blob/master/yaml/xyz/openbmc_project/State/Decorator/Availability.interface.yaml
[3]:
  https://github.com/openbmc/phosphor-dbus-interfaces/blob/master/yaml/xyz/openbmc_project/Object/Enable.interface.yaml
[4]:
  https://github.com/openbmc/phosphor-dbus-interfaces/blob/master/yaml/xyz/openbmc_project/State/Decorator/PowerSystemInputs.interface.yaml
