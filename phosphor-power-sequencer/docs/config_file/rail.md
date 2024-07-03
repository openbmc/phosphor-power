# rail

## Description

A voltage rail that is enabled or monitored by the power sequencer device.

The "check_status_vout", "compare_voltage_to_limit", and "gpio" properties
specify how to obtain the pgood status of the rail. You can specify more than
one of these properties.

The "check_status_vout" method is usually the most accurate. For example, if a
pgood fault occurs, the power sequencer device may automatically shut off
related rails. Ideally the device will only set fault bits in STATUS_VOUT for
the rail with the pgood fault. However, all the related rails will likely appear
to be faulted by the other methods.

The "compare_voltage_to_limit" method is helpful when a rail fails to power on
during the power on sequence and the power sequencer device waits indefinitely
for it to power on.

The "gpio" method is necessary when no PMBus information is available for the
rail. It is also helpful when a rail fails to power on during the power on
sequence and the power sequencer device waits indefinitely for it to power on.

## Properties

| Name                     |      Required       | Type                    | Description                                                                                                                                                                                                                                                                                                                    |
| :----------------------- | :-----------------: | :---------------------- | :----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| name                     |         yes         | string                  | Unique name for the rail. Can only contain letters (A-Z, a-z), numbers (0-9), period (.), and underscore (\_).                                                                                                                                                                                                                 |
| presence                 |         no          | string                  | D-Bus inventory path of a system component which must be present in order for the rail to be present. If this property is not specified, the rail is assumed to always be present.                                                                                                                                             |
| page                     | see [notes](#notes) | number                  | PMBus PAGE number of the rail.                                                                                                                                                                                                                                                                                                 |
| is_power_supply_rail     |         no          | boolean (true or false) | If true, this rail is produced by a power supply. Power supply rails require special error handling. If an error is detected in a power supply device, and the pgood status is false for a power supply rail, the power supply error is logged as the cause of the pgood failure. The default value of this property is false. |
| check_status_vout        |         no          | boolean (true or false) | If true, determine the pgood status of the rail by checking the value of the PMBus STATUS_VOUT command. If one of the error bits is set, the rail pgood will be considered false. The default value of this property is false.                                                                                                 |
| compare_voltage_to_limit |         no          | boolean (true or false) | If true, determine the pgood status of the rail by comparing the output voltage (READ_VOUT) to the undervoltage fault limit (VOUT_UV_FAULT_LIMIT). If the output voltage is below this limit, the rail pgood will be considered false. The default value of this property is false.                                            |
| gpio                     |         no          | [gpio](gpio.md)         | Determine the pgood status of the rail by reading a GPIO.                                                                                                                                                                                                                                                                      |

### Notes

- The "page" property is required if the "check_status_vout" or
  "compare_voltage_to_limit" property is true.

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
