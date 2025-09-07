# Powering Off

## Initiating a power off

The system can be powered off by several methods, such as the `obmcutil` tool, a
Redfish command, or a power button on the system enclosure.

Whichever method is used, it sets the `state` property to 0 on the
`org.openbmc.control.Power` D-Bus interface. The D-Bus object path is
`/org/openbmc/control/power0`, which represents the entire system. See
[Chassis Status](chassis_status.md) for more information.

The `phosphor-power-sequencer` application only supports powering off the entire
system. In a multiple chassis system, `phosphor-power-sequencer` does not
support powering off individual chassis independent of the rest of the system.

## Determining which chassis to power off

In a single chassis system, `phosphor-power-sequencer` will always attempt to
power off the chassis.

In a multiple chassis system, `phosphor-power-sequencer` will only attempt to
power off chassis with the proper status:

- `Present` property is true
- `Available` property is true (if interface exists)

`phosphor-power-sequencer` will set the `state` property to 0 for all chassis.

`phosphor-power-sequencer` will set the `pgood` property to 0 for all chassis
where `Present` or `Available` are false.

See [Chassis Status](chassis_status.md) for more information on these
properties.

## Powering off the voltage rails

`phosphor-power-sequencer` powers off the main (non-standby) voltage rails in a
chassis by toggling a named GPIO to the power sequencer device in the chassis.
For more information, see [Named GPIOs](named_gpios.md).

In each chassis being powered off, the power sequencer device powers off the
individual voltage rails in the correct order.

## Determining when power off is complete

When the voltage rails have been powered off in a chassis, the power sequencer
device will set the chassis power good (pgood) signal to false.

`phosphor-power-sequencer` reads the chassis pgood signal from a named GPIO. For
more information, see
[Monitoring Chassis Power Good](monitoring_chassis_pgood.md).

When the chassis power good signal changes to false, `phosphor-power-sequencer`
will set the `pgood` property to 0 on the `org.openbmc.control.Power` interface
for the **chassis** object path. The power off has finished for that chassis.

When all chassis that were being powered off have finished,
`phosphor-power-sequencer` will set the `pgood` property to 0 on the
`org.openbmc.control.Power` interface for the **system** object path.

See [Chassis Status](chassis_status.md) for more information on the `pgood`
property.
