# compare_vpd

## Description

Compares a VPD (Vital Product Data) keyword value to an expected value.

VPD is information that describes a hardware component. VPD is typically read
from an EEPROM on a Field-Replaceable Unit (FRU). For this reason, VPD is also
called "FRU data".

The phosphor-regulators application obtains VPD keyword values from D-Bus. Other
BMC applications and drivers are responsible for reading VPD from hardware
components and publishing it on D-Bus.

The following VPD keywords are currently supported:

- CCIN
- Manufacturer
- Model
- PartNumber
- HW

This action can be used in an [if](if.md) condition to execute actions based on
a VPD keyword value. For example, you could set the output voltage only for
regulators with a specific Model number.

### Unavailable keyword values

A keyword value may be unavailable if:

- The hardware component does not support the keyword.
- An error occurred while attempting to read VPD from the hardware component.
- The BMC cannot access the VPD due to the system hardware design, such as not
  being connected to the necessary I2C bus.

If the keyword value is unavailable, it will be treated as having an empty
string ("") value.

If the expected value is not an empty string, the compare_vpd action will return
false since the values will not match.

## Properties

| Name    | Required | Type   | Description                                                                                                                                                                                                                                                             |
| :------ | :------: | :----- | :---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| fru     |   yes    | string | Field-Replaceable Unit (FRU) that contains the VPD. Specify the relative D-Bus inventory path of the FRU. Full inventory paths begin with the root "/xyz/openbmc_project/inventory". Specify the relative path below the root, such as "system/chassis/disk_backplane". |
| keyword |   yes    | string | VPD keyword. Specify one of the following: "CCIN", "Manufacturer", "Model", "PartNumber", "HW".                                                                                                                                                                         |
| value   |   yes    | string | Expected value.                                                                                                                                                                                                                                                         |

## Return Value

Returns true if the keyword value equals the expected value, otherwise returns
false.

## Examples

```json
{
  "comments": ["Check if disk backplane has CCIN value 2D35"],
  "compare_vpd": {
    "fru": "system/chassis/disk_backplane",
    "keyword": "CCIN",
    "value": "2D35"
  }
}
```
