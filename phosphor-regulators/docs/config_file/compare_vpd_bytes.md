# compare_vpd_bytes

## Description

Compares a VPD (Vital Product Data) keyword value to an array of expected byte
values. See [Compare VPD Overview](compare_vpd_overview.md) for more information
on VPD, supported keywords, and unavailable keyword values.

This action can be used in an [if](if.md) condition to execute actions based on
a VPD keyword value. For example, you could set the output voltage only for
regulators with a specific Model number.

The number of bytes in the VPD keyword value must equal the number of expected
byte values. If the numbers differ, the comparison will return false.

If the optional "masks" property is specified, only the bits with a value of 1
in each mask will be compared. Otherwise all bits are compared.

## Properties

| Name    | Required | Type             | Description                                                                                                                                                                                                                                                                                                                                     |
| :------ | :------: | :--------------- | :---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| fru     |   yes    | string           | Field-Replaceable Unit (FRU) that contains the VPD. Specify the relative D-Bus inventory path of the FRU. Full inventory paths begin with the root "/xyz/openbmc_project/inventory". Specify the relative path below the root, such as "system/chassis/disk_backplane".                                                                         |
| keyword |   yes    | string           | VPD keyword. Specify one of the following: "CCIN", "Manufacturer", "Model", "PartNumber", "HW".                                                                                                                                                                                                                                                 |
| values  |   yes    | array of strings | Zero or more expected byte values expressed in hexadecimal. Each value must be prefixed with 0x and surrounded by double quotes.                                                                                                                                                                                                                |
| masks   |    no    | array of strings | One or more bit masks expressed in hexadecimal. Each mask must be prefixed with 0x and surrounded by double quotes. The number of bit masks must match the number of expected byte values. Each mask specifies which bits should be compared within the corresponding byte value. Only the bits with a value of 1 in the mask will be compared. |

## Return Value

Returns true if the keyword value bytes equal the expected value bytes,
otherwise returns false.

## Examples

```json
{
  "comments": [
    "Check if disk backplane has CCIN byte values 0x32, 0x44, 0x33, 0x35"
  ],
  "compare_vpd_bytes": {
    "fru": "system/chassis/disk_backplane",
    "keyword": "CCIN",
    "values": ["0x32", "0x44", "0x33", "0x35"]
  }
}
```

```json
{
  "comments": [
    "Check if HW keyword has byte values 0x73, 0x02.",
    "Ignore the most significant bit in each byte."
  ],
  "compare_vpd_bytes": {
    "fru": "system/chassis/disk_backplane",
    "keyword": "HW",
    "values": ["0x73", "0x02"],
    "masks": ["0x7F", "0x7F"]
  }
}
```
