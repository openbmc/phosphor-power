# Powering On

## Initiating a power on

The system can be powered on by several methods, such as the `obmcutil` tool, a
Redfish command, or a power button on the system enclosure.

Whichever method is used, it sets the `state` property to 1 on the
`org.openbmc.control.Power` D-Bus interface. The D-Bus object path is
`/org/openbmc/control/power0`, which represents the entire system. See
[Chassis Status](chassis_status.md) for more information.

The `phosphor-power-sequencer` application only supports powering on the entire
system. In a multiple chassis system, `phosphor-power-sequencer` does not
support powering on individual chassis independent of the rest of the system.

## Determining which chassis to power on

In a single chassis system, `phosphor-power-sequencer` will always attempt to
power on the chassis.

In a multiple chassis system, `phosphor-power-sequencer` will only attempt to
power on chassis with the proper status:

- `Present` property is true
- `Enabled` property is true (if interface exists)
- `Available` property is true (if interface exists)
- `Status` property is `Good` (if interface exists)

`phosphor-power-sequencer` will set the `state` property to 1 for each chassis
that is being powered on. It will set `state` to 0 for each chassis not being
powered on.

If no chassis are in the proper status to power on, `phosphor-power-sequencer`
will log an error. `phosphor-chassis-state-manager` will
[power off](powering_off.md) the system.

See [Chassis Status](chassis_status.md) for more information on these
properties.

## Powering on the voltage rails

`phosphor-power-sequencer` powers on the main (non-standby) voltage rails in a
chassis by toggling a named GPIO to the power sequencer device in the chassis.
For more information, see [Named GPIOs](named_gpios.md).

In each chassis being powered on, the power sequencer device powers on the
individual voltage rails in the correct order.

## Determining when power on is complete

When all voltage rails have been successfully powered on in a chassis, the power
sequencer device will set the chassis power good (pgood) signal to true.

`phosphor-power-sequencer` reads the chassis pgood signal from a named GPIO. For
more information, see
[Monitoring Chassis Power Good](monitoring_chassis_pgood.md).

When the chassis power good signal changes to true, `phosphor-power-sequencer`
will set the `pgood` property to 1 on the `org.openbmc.control.Power` interface
for the **chassis** object path. The power on has finished for that chassis.

When all chassis that were being powered on have finished,
`phosphor-power-sequencer` will set the `pgood` property to 1 on the
`org.openbmc.control.Power` interface for the **system** object path.

See [Chassis Status](chassis_status.md) for more information on the `pgood`
property.

After all chassis have powered on, the rest of the boot process continues. The
host operating system will eventually be started.

## Handling errors

### Power good fault

When the power sequencer device is powering on the main voltage rails, one of
the rails may fail to power on. Similarly, after the system has powered on, one
of the voltage rails that had been providing power to the chassis might suddenly
power off.

In both cases the result is a pgood fault. See
[Power Good Faults](pgood_faults.md) for information on how the error is
handled.

### Unable to read chassis power good signal

`phosphor-power-sequencer` may become unable to read the chassis power good
signal from the named GPIO due to:

- Hardware communication problems.
- The `Available` property of the chassis changes to false.

See [Monitoring Chassis Power Good](monitoring_chassis_pgood.md) for more
information on how the error is handled.
