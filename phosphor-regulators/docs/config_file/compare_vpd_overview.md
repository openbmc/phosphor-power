# Compare VPD Overview

VPD (Vital Product Data) is information that describes a hardware component. VPD
is typically read from an EEPROM on a Field-Replaceable Unit (FRU). For this
reason, VPD is also called "FRU data".

The phosphor-regulators application obtains VPD keyword values from D-Bus. Other
BMC applications and drivers are responsible for reading VPD from hardware
components and publishing it on D-Bus.

A VPD value can be compared to an expected value. The result can be used to
perform configuration or monitoring differently based on the hardware
type/version. For example, a rail could be set to a lower voltage for an older
version of a processor.

## Supported Actions

- [compare_vpd](compare_vpd.md)
- [compare_vpd_byte](compare_vpd_byte.md)
- [compare_vpd_bytes](compare_vpd_bytes.md)

## D-Bus interface

### Default interface

The default D-Bus interface used to find the VPD keyword is
[xyz.openbmc_project.Inventory.Decorator.Asset](https://github.com/openbmc/phosphor-dbus-interfaces/blob/master/yaml/xyz/openbmc_project/Inventory/Decorator/Asset.interface.yaml)

This interface provides the following VPD keywords:

- PartNumber
- SerialNumber
- Manufacturer
- BuildDate
- Model
- SubModel
- SparePartNumber
- SKU

The "CCIN" keyword can also be used with this interface; it is another name for
the "Model" keyword.

### Other D-Bus interfaces

The desired VPD keyword may be published on a different interface, such as
"com.acme.vpd". Use the "interface" JSON property to specify the D-Bus
interface.

## Unavailable Keyword Values

A keyword value may be unavailable if:

- The hardware component does not support the keyword.
- An error occurred while attempting to read VPD from the hardware component.
- The BMC cannot access the VPD due to the system hardware design, such as not
  being connected to the necessary I2C bus.

If the keyword value is unavailable, it will be treated as an empty value:

- [compare_vpd](compare_vpd.md) treats it as an empty string (`""`).
- [compare_vpd_byte](compare_vpd_byte.md) treats it as an empty byte array
  (`[]`). Since any byte position is past the end of the array, the comparison
  will return false.
- [compare_vpd_bytes](compare_vpd_bytes.md) treats it as an empty byte array
  (`[]`).

If the expected value is not empty, the comparison will return false since the
values will not match.
