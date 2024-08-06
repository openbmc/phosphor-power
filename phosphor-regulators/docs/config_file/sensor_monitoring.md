# sensor_monitoring

## Description

Defines how to read the sensors for a voltage rail, such as voltage output,
current output, and temperature. Sensor values are measured, actual values
rather than target values.

Sensors will be read once per second. The sensor values will be stored on D-Bus
on the BMC, making them available to external interfaces like Redfish.

The [pmbus_read_sensor](pmbus_read_sensor.md) action is used to read one sensor.
To read multiple sensors, multiple "pmbus_read_sensor" actions need to be
executed.

The "pmbus_read_sensor" actions can be specified in two ways:

- Use the "rule_id" property to specify a standard rule to run.
- Use the "actions" property to specify an array of actions that are unique to
  this device.

## Properties

| Name     |      Required       | Type                          | Description                                                 |
| :------- | :-----------------: | :---------------------------- | :---------------------------------------------------------- |
| comments |         no          | array of strings              | One or more comment lines describing the sensor monitoring. |
| rule_id  | see [notes](#notes) | string                        | Unique ID of the [rule](rule.md) to execute.                |
| actions  | see [notes](#notes) | array of [actions](action.md) | One or more actions to execute.                             |

### Notes

- You must specify either "rule_id" or "actions".

## Examples

```json
{
  "comments": ["Read all sensors supported by the IR35221 regulator"],
  "rule_id": "read_ir35221_sensors_rule"
}
```

```json
{
  "comments": [
    "Only read sensors if version register 0x75 contains 2.",
    "Earlier versions produced invalid sensor values."
  ],
  "actions": [
    {
      "if": {
        "condition": {
          "i2c_compare_byte": { "register": "0x75", "value": "0x02" }
        },
        "then": [{ "run_rule": "read_sensors_rule" }]
      }
    }
  ]
}
```
