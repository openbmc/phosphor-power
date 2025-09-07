# Monitoring Chassis Power Good

## Overview

The power sequencer device provides a chassis power good (pgood) signal. This
indicates that all of the main (non-standby) voltage rails are powered on.

The `phosphor-power-sequencer` application periodically reads (polls) the
chassis pgood signal from a named GPIO. For more information, see
[Named GPIOs](named_gpios.md).

The resulting chassis pgood value is used to set the `pgood` property for the
chassis. See [Chassis Status](chassis_status.md) for more information on this
property.

If the chassis pgood state is false when it should be true, a chassis power good
(pgood) fault has occurred. See [Power Good Faults](pgood_faults.md) for more
information.

## Unable to read chassis power good signal

`phosphor-power-sequencer` may become unable to read the chassis power good
signal from the named GPIO due to:

- Hardware communication problems
  - Unable to read from named GPIO after multiple retries.
- The `Available` property of the chassis changes to false.

### During power on

If `phosphor-power-sequencer` is unable to read the chassis power good signal
during a power on attempt, the application will do the following:

- If this is a single chassis system:
  - Log an error specifying the power on attempt failed due to an inability to
    read the chassis power good status.
  - `phosphor-chassis-state-manager` will [power off](powering_off.md) the
    system.
- If this is a multiple chassis system:
  - If the chassis is experiencing a blackout:
    - Log an error specifying an input power problem.
  - If the chassis is not experiencing a blackout:
    - Log an error specifying the power on attempt failed due to an inability to
      read the chassis power good status.
    - If the read failure was due to hardware communication problems, add the
      inventory path of the chassis to the error log. This may result in
      hardware isolation, which will cause the `Enabled` property of the chassis
      to be false.
  - `phosphor-chassis-state-manager` will power the system
    [off](powering_off.md) and then [on](powering_on.md) again. If the chassis's
    `Available` or `Enabled` property is false, the chassis will not be powered
    on.

### After power on

If `phosphor-power-sequencer` is unable to read the chassis power good signal
after the chassis powered on, the application will do the following:

- If this is a single chassis system:
  - Log an error specifying an inability to read the chassis power good status.
  - Do **not** power off the system.
  - Leave the `pgood` property set to the value 1.
- If this is a multiple chassis system:
  - If the chassis is experiencing a blackout:
    - Set `pgood` value to 0.
    - Log an error specifying an input power problem.
    - Power the system off and then on again. If the chassis's `Available`
      property is false, the chassis will not be powered on.
  - If the chassis is not experiencing a blackout:
    - Log an error specifying an inability to read the chassis power good
      status.
    - Do **not** power off the system.
    - Leave the `pgood` property set to the value 1.
