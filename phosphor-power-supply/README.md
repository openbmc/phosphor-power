OpenBMC power supply monitoring application.

Design document: https://github.com/openbmc/docs/blob/master/designs/psu-monitoring.md

# Compile Options

To enable reading VPD data via PMBus commands to IBM common form factor
power supplies (ibm-cffps), run meson with `-Dibm-vpd=true`.

# JSON Configuration File

The JSON configuration file should contain:

## MinPowerSupplies
Optional property, integer, that indicates the minimum number of power supplies
that should be present.

## MaxPowerSupplies
Optional property, integer, that indicates the maximum number of power supplies
that should be present.

## PowerSupplies
An array of power supply properties.

### Inventory
The D-Bus path used to check the power supply presence (Present) property.

### Bus
An integer specifying the I2C bus that the PMBus power supply is on.

### Address
A string representing the 16-bit I2C address of the PMBus power supply.

