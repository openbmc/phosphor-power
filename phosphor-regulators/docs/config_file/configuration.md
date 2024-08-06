# configuration

## Description

Configuration changes that should be applied to a device or regulator rail.
These changes usually override hardware default settings.

The most common configuration change is setting the output voltage for a
regulator rail. Other examples include modifying pgood thresholds and
overcurrent settings.

The configuration changes are applied during the boot before regulators are
enabled.

The configuration changes are applied by executing one or more actions. The
actions can be specified in two ways:

- Use the "rule_id" property to specify a standard rule to run.
- Use the "actions" property to specify an array of actions that are unique to
  this device.

## Properties

| Name     |      Required       | Type                          | Description                                                                                                                     |
| :------- | :-----------------: | :---------------------------- | :------------------------------------------------------------------------------------------------------------------------------ |
| comments |         no          | array of strings              | One or more comment lines describing the configuration changes.                                                                 |
| volts    |         no          | number                        | Output voltage expressed as a decimal number. Applied using the [pmbus_write_vout_command](pmbus_write_vout_command.md) action. |
| rule_id  | see [notes](#notes) | string                        | Unique ID of the [rule](rule.md) to execute.                                                                                    |
| actions  | see [notes](#notes) | array of [actions](action.md) | One or more actions to execute.                                                                                                 |

### Notes

- You must specify either "rule_id" or "actions".

## Examples

```json
{
  "comments": ["Set rail to 1.25V using standard rule"],
  "volts": 1.25,
  "rule_id": "set_voltage_rule"
}
```

```json
{
  "comments": [
    "If version register 0x75 contains 1, device is downlevel",
    "and registers 0x31/0x34 need to be updated."
  ],
  "actions": [
    {
      "if": {
        "condition": {
          "i2c_compare_byte": { "register": "0x75", "value": "0x01" }
        },
        "then": [
          { "i2c_write_byte": { "register": "0x31", "value": "0xDB" } },
          { "i2c_write_byte": { "register": "0x34", "value": "0x1B" } }
        ]
      }
    }
  ]
}
```
