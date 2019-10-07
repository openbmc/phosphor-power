# compare_vpd

## Description
Compares a VPD (Vital Product Data) keyword value to an expected value.

The following keywords are currently supported:
* CCIN
* Manufacturer
* Model
* PartNumber

## Properties
| Name | Required | Type | Description |
| :--- | :------: | :--- | :---------- |
| fru | yes | string | Field-Replaceable Unit (FRU) that contains the VPD.  Specify the D-Bus inventory path of the FRU. |
| keyword | yes | string | VPD keyword.  Specify one of the following: "CCIN", "Manufacturer", "Model", "PartNumber". |
| value | yes | string | Expected value. |

## Return Value
Returns true if the keyword value equals the expected value, otherwise returns
false.

## Example
```
{
  "comments": [ "Check if disk backplane has CCIN value 2D35" ],
  "compare_vpd": {
    "fru": "/system/chassis/disk_backplane",
    "keyword": "CCIN",
    "value": "2D35"
  }
}
```
