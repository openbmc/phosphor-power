# action

## Description

Action to execute.

Actions are executed to perform the following regulator operations:

- [presence_detection](presence_detection.md)
- [configuration](configuration.md)
- [sensor_monitoring](sensor_monitoring.md)
- [phase_fault_detection](phase_fault_detection.md)

Many actions read from or write to a hardware device. Initially this is the
[device](device.md) that contains the regulator operation. However, the device
can be changed using the [set_device](set_device.md) action.

## Properties

| Name                     |      Required       | Type                                                    | Description                                                          |
| :----------------------- | :-----------------: | :------------------------------------------------------ | :------------------------------------------------------------------- |
| comments                 |         no          | array of strings                                        | One or more comment lines describing this action.                    |
| and                      | see [notes](#notes) | array of actions                                        | Action type [and](and.md).                                           |
| compare_presence         | see [notes](#notes) | [compare_presence](compare_presence.md)                 | Action type [compare_presence](compare_presence.md).                 |
| compare_vpd              | see [notes](#notes) | [compare_vpd](compare_vpd.md)                           | Action type [compare_vpd](compare_vpd.md).                           |
| i2c_capture_bytes        | see [notes](#notes) | [i2c_capture_bytes](i2c_capture_bytes.md)               | Action type [i2c_capture_bytes](i2c_capture_bytes.md).               |
| i2c_compare_bit          | see [notes](#notes) | [i2c_compare_bit](i2c_compare_bit.md)                   | Action type [i2c_compare_bit](i2c_compare_bit.md).                   |
| i2c_compare_byte         | see [notes](#notes) | [i2c_compare_byte](i2c_compare_byte.md)                 | Action type [i2c_compare_byte](i2c_compare_byte.md).                 |
| i2c_compare_bytes        | see [notes](#notes) | [i2c_compare_bytes](i2c_compare_bytes.md)               | Action type [i2c_compare_bytes](i2c_compare_bytes.md).               |
| i2c_write_bit            | see [notes](#notes) | [i2c_write_bit](i2c_write_bit.md)                       | Action type [i2c_write_bit](i2c_write_bit.md).                       |
| i2c_write_byte           | see [notes](#notes) | [i2c_write_byte](i2c_write_byte.md)                     | Action type [i2c_write_byte](i2c_write_byte.md).                     |
| i2c_write_bytes          | see [notes](#notes) | [i2c_write_bytes](i2c_write_bytes.md)                   | Action type [i2c_write_bytes](i2c_write_bytes.md).                   |
| if                       | see [notes](#notes) | [if](if.md)                                             | Action type [if](if.md).                                             |
| log_phase_fault          | see [notes](#notes) | [log_phase_fault](log_phase_fault.md)                   | Action type [log_phase_fault](log_phase_fault.md).                   |
| not                      | see [notes](#notes) | action                                                  | Action type [not](not.md).                                           |
| or                       | see [notes](#notes) | array of actions                                        | Action type [or](or.md).                                             |
| pmbus_read_sensor        | see [notes](#notes) | [pmbus_read_sensor](pmbus_read_sensor.md)               | Action type [pmbus_read_sensor](pmbus_read_sensor.md).               |
| pmbus_write_vout_command | see [notes](#notes) | [pmbus_write_vout_command](pmbus_write_vout_command.md) | Action type [pmbus_write_vout_command](pmbus_write_vout_command.md). |
| run_rule                 | see [notes](#notes) | string                                                  | Action type [run_rule](run_rule.md).                                 |
| set_device               | see [notes](#notes) | string                                                  | Action type [set_device](set_device.md).                             |

### Notes

- You must specify exactly one action type property, such as "i2c_write_byte" or
  "run_rule".

## Return Value

When the action completes, it returns a true or false value. The documentation
for the specified action type describes what value will be returned.

## Examples

```json
{
  "comments": ["Set frequency to 800kHz"],
  "i2c_write_byte": {
    "register": "0x2C",
    "value": "0x0F"
  }
}
```

```json
{
  "run_rule": "set_voltage_rule"
}
```
