# Power Loss

## Overview

Power distribution in a computer system is complex. It typically flows from a
wall outlet to power supplies to voltage regulators to system components. Other
devices may exist between the wall outlet and the power supplies, such as an
Uninterruptible Power Supply (UPS) or an Enclosure Power Distribution Unit
(ePDU).

A **brownout** is a partial reduction in voltage to the power supplies. In some
situations, the standby voltage rails are still powered on but the main voltage
rails are powered off. As a result, the BMC may still be running, but the host
processor and other system components have lost power.

A **blackout** is a complete loss of power to the power supplies.

In a multiple chassis system, a brownout or blackout might only occur in some of
the chassis.

## System behavior during a brownout

If the chassis was powered off when the brownout occurred, the
`phosphor-power-sequencer` application will take no action.

If the chassis was powered on when the brownout occurred, the power sequencer
device will normally change the chassis power good (pgood) signal from true to
false. `phosphor-power-sequencer` will isolate the failure to the power supply
rail. `phosphor-power-sequencer` will log a power supply error. This error will
specify the problem was the input voltage rather than the power supply hardware.
See [Power Good Faults](pgood_faults.md) for more information.

## System behavior during a blackout

### Single chassis system

The system loses all power. It will be completely off until utility power is
restored.

When power is restored, if the system was previously powered on, it may be
automatically powered on again by the `phosphor-chassis-state-manager`
application. This depends on the Auto Power Restart settings.

### Multiple chassis system

If the blackout affects all chassis, the system loses all power. It will behave
as described above for a single chassis system.

If the blackout only affects some chassis, the following steps will occur:

- The `phosphor-chassis-power` application will do the following:
  - Detect which chassis are experiencing a blackout.
  - Set the `Status` property of the
    `xyz.openbmc_project.State.Decorator.PowerSystemInputs` D-Bus interface to
    `Fault` for the chassis experiencing a blackout.
- The `Available` property will be set to false for chassis that are
  experiencing a blackout.
  - This is due to the `PowerSystemInputs` `Status` property being set to
    `Fault` for the chassis.
- If the BMC was reset by hardware due to the blackout, the following will
  occur:
  - `phosphor-power-sequencer` will read the pgood signal for each chassis and
    use that value for the `state` and `pgood` properties of the
    `org.openbmc.control.Power` D-Bus interface for the chassis.
    - For chassis experiencing a blackout, the `state` and `pgood` properties
      will be set to 0.
  - `phosphor-chassis-state-manager` will obtain the previous on/off state of
    each chassis from saved data.
  - `phosphor-chassis-state-manager` will determine if a power problem has
    occurred by checking if the following are all true:
    - At least one chassis was previously on and is now off.
    - The host operating system is not running or communicating
  - If a power problem has occurred, `phosphor-chassis-state-manager` will do
    the following:
    - Log an error
    - Power the system [off](powering_off.md) and then [on](powering_on.md)
      again. The chassis with `Available` set to false will **not** be powered
      on.

See [Chassis Status](chassis_status.md) for more information on the D-Bus
chassis status properties.
