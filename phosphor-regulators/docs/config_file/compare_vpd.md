# compare_vpd

## Description

Compares a VPD (Vital Product Data) keyword value to an expected value. See
[Compare VPD Overview](compare_vpd_overview.md) for more information on VPD,
supported keywords, and unavailable keyword values.

This action can be used in an [if](if.md) condition to execute actions based on
a VPD keyword value. For example, you could set the output voltage only for
regulators with a specific Model number.

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
