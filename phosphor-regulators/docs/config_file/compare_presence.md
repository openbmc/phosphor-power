# compare_presence

## Description
Compares a hardware component's presence to an expected value.  The hardware
component must be a Field-Replaceable Unit (FRU).

This action can be used for [presence_detection](presence_detection.md) of a
[device](device.md).  It can also be used in an [if](if.md) condition to
execute actions based on FRU presence.

## Properties
| Name | Required | Type | Description |
| :--- | :------: | :--- | :---------- |
| fru | yes | string | Field-Replaceable Unit.  Specify the D-Bus inventory path of the FRU. |
| value | yes | boolean (true or false) | Expected presence value. |

## Return Value
Returns true if the actual presence value equals the expected value, otherwise
returns false.

## Example
```
{
  "comments": [ "Check if CPU3 is present" ],
  "compare_presence": {
    "fru": "/system/chassis/motherboard/cpu3",
    "value": true
  }
}
```
