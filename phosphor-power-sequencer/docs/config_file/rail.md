# rail

## Description

A voltage rail that is enabled or monitored by the power sequencer device.

The "check_status_vout", "compare_voltage_to_limits", and "gpio" properties
specify how to obtain the pgood status of the rail.  You can specify more than
one of these properties if necessary.

## Properties

| Name                      |      Required       | Type                    | Description |
| :------------------------ | :-----------------: | :---------------------- | :---------- |
| name                      |        yes          | string                  | Unique name for the rail.  Can only contain letters (A-Z, a-z), numbers (0-9), period (.), and underscore (\_). |
| presence                  |         no          | string                  | D-Bus inventory path of a system component which must be present in order for the rail to be present.  If this property is not specified, the rail is assumed to always be present. |
| page                      | see [notes](#notes) | number                  | PMBus PAGE number of the rail. |
| check_status_vout         |         no          | boolean (true or false) | If true, determine the pgood status of the rail by checking the value of the PMBus STATUS_VOUT command.  If one of the error bits is set, the rail pgood will be considered false.  The default value of this property is false. |
| compare_voltage_to_limits |         no          | boolean (true or false) | If true, determine the pgood status of the rail by comparing the output voltage (READ_VOUT) to the undervoltage (VOUT_UV_FAULT_LIMIT) and overvoltage (VOUT_OV_FAULT_LIMIT) limits.  If the output voltage is beyond those limits, the rail pgood will be considered false.  The default value of this property is false. |
| gpio                      |         no          | [gpio](gpio.md)         | Determine the pgood status of the rail by reading a GPIO. |

### Notes

- The "page" property is required if the "check_status_vout" or
  "compare_voltage_to_limits" property is specified.

## Examples

```
{
    "name": "VDD_CPU0",
    "page": 11,
    "check_status_vout": true
}
```

```
{
    "name": "VCS_CPU1",
    "presence": "/xyz/openbmc_project/inventory/system/chassis/motherboard/cpu1",
    "gpio": { "line": 60 }
}
```
