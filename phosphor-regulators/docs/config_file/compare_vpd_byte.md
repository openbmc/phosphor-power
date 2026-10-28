# compare_vpd_byte

## Description

Compares a byte within a VPD (Vital Product Data) keyword value to an expected
byte value. See [Compare VPD Overview](compare_vpd_overview.md) for more
information on VPD, supported keywords, and unavailable keyword values.

This action can be used in an [if](if.md) condition to execute actions based on
a VPD keyword value. For example, you could set the output voltage only for
regulators with a specific hardware revision number.

The "position" property specifies which byte to compare within the VPD keyword
value. Byte positions are zero-based; position 0 is the first byte. If the byte
position is past the end of the VPD value byte array (or if the VPD value is
empty/unavailable), the comparison will return false.

If the optional "mask" property is specified, only the bits with a value of 1 in
the mask will be compared. Otherwise all bits are compared.

## Properties

| Name     | Required | Type   | Description                                                                                                                                                                                                                                                             |
| :------- | :------: | :----- | :---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| fru      |   yes    | string | Field-Replaceable Unit (FRU) that contains the VPD. Specify the relative D-Bus inventory path of the FRU. Full inventory paths begin with the root "/xyz/openbmc_project/inventory". Specify the relative path below the root, such as "system/chassis/disk_backplane". |
| keyword  |   yes    | string | VPD keyword. Specify one of the following: "CCIN", "Manufacturer", "Model", "PartNumber", "HW".                                                                                                                                                                         |
| position |   yes    | number | Byte position within the VPD keyword value. Zero-based; 0 indicates the first byte.                                                                                                                                                                                     |
| value    |   yes    | string | Expected byte value expressed in hexadecimal. Must be prefixed with 0x and surrounded by double quotes.                                                                                                                                                                 |
| mask     |    no    | string | Bit mask expressed in hexadecimal. Must be prefixed with 0x and surrounded by double quotes. Specifies which bits should be compared within the byte value. Only the bits with a value of 1 in the mask will be compared.                                               |

## Return Value

Returns true if the keyword value byte equals the expected value byte, otherwise
returns false.

## Examples

```json
{
  "comments": ["Check if CCIN first byte is 0x32"],
  "compare_vpd_byte": {
    "fru": "system/chassis/disk_backplane",
    "keyword": "CCIN",
    "position": 0,
    "value": "0x32"
  }
}
```

```json
{
  "comments": [
    "Check if HW keyword byte at index 1 is 0x02.",
    "Ignore the most significant bit."
  ],
  "compare_vpd_byte": {
    "fru": "system/chassis/disk_backplane",
    "keyword": "HW",
    "position": 1,
    "value": "0x02",
    "mask": "0x7F"
  }
}
```
