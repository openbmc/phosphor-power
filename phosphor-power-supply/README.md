# OpenBMC power supply monitoring application

Design document:
<https://github.com/openbmc/docs/blob/master/designs/psu-monitoring.md>

## Compile Options

To enable reading VPD data via PMBus commands to IBM common form factor power
supplies (ibm-cffps), run meson with `-Dibm-vpd=true`.

## D-Bus System Configuration

Entity Manager provides information about the supported system configuration and
the power supply connectors (IBMCFFPSConnector).

The information is as follows:

### Max Power Supplies

Integer that indicates the maximum number of power supplies that should be
present. This is exposed via the `MaxCount` property.

### I2C Bus

The I2C bus(es) that the power supply is on will be represented by the `I2CBus`
property under the `xyz.openbmc_project.Configuration.IBMCFFPSConnector`
interface(s).

### I2C Address

The I2C address(es) that the power supply is at will be represented by the
`I2CAddress` property under the IBMCFFPSConnector interface(s).

### Name

The `Name` property under the IBMCFFPSConnector interface(s) will be used to
create an inventory path for the power supply. This inventory path is used as
part of the power supply presence detection, reading the `Present` property
under this path.
