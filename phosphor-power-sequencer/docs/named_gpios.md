# Named GPIOs

## Chassis power control

The main (non-standby) voltage rails in a chassis are powered on or off by
toggling a named GPIO to the power sequencer device in the chassis.

The GPIO name is defined in the Linux device tree. For single chassis systems,
the standard GPIO name is `power-chassis-control`. See
[Device Tree GPIO Naming in OpenBMC](https://github.com/openbmc/docs/blob/master/designs/device-tree-gpio-naming.md)
for more information.

The GPIO name in each chassis is specified in the
[JSON configuration file](config_file/README.md). If no configuration file is
found for the current system, the standard GPIO name is used.

## Chassis power good (pgood)

The power sequencer device provides a chassis power good (pgood) signal,
indicating whether all the main (non-standby) rails are powered on.

The `phosphor-power-sequencer` application reads the chassis power good signal
from a named GPIO.

The GPIO name is defined in the Linux device tree. For single chassis systems,
the standard GPIO name is `power-chassis-good`. See
[Device Tree GPIO Naming in OpenBMC](https://github.com/openbmc/docs/blob/master/designs/device-tree-gpio-naming.md)
for more information.

The GPIO name in each chassis is specified in the
[JSON configuration file](config_file/README.md). If no configuration file is
found for the current system, the standard GPIO name is used.
