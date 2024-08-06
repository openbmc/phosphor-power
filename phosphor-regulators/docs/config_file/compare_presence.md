# compare_presence

## Description

Compares a hardware component's presence to an expected value. The hardware
component must be a Field-Replaceable Unit (FRU).

This action can be used for [presence_detection](presence_detection.md) of a
[device](device.md). It can also be used in an [if](if.md) condition to execute
actions based on FRU presence.

## Properties

| Name  | Required | Type                    | Description                                                                                                                                                                                                                                   |
| :---- | :------: | :---------------------- | :-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| fru   |   yes    | string                  | Field-Replaceable Unit. Specify the relative D-Bus inventory path of the FRU. Full inventory paths begin with the root "/xyz/openbmc_project/inventory". Specify the relative path below the root, such as "system/chassis/motherboard/cpu3". |
| value |   yes    | boolean (true or false) | Expected presence value.                                                                                                                                                                                                                      |

## Return Value

Returns true if the actual presence value equals the expected value, otherwise
returns false.

## Example

```json
{
  "comments": ["Check if CPU3 is present"],
  "compare_presence": {
    "fru": "system/chassis/motherboard/cpu3",
    "value": true
  }
}
```
