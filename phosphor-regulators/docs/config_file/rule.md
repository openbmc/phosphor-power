# rule

## Description

A rule is a sequence of actions that can be shared by multiple regulators in the
config file. Rules define a standard way to perform an operation. Rules are used
to minimize duplication in the config file.

For example, the following action sequences might be sharable using a rule:

- Actions that set the output voltage of a regulator rail
- Actions that read all the sensors of a regulator rail
- Actions that detect down-level hardware using version registers
- Actions that detect phase faults

## Properties

| Name     | Required | Type                          | Description                                                                                       |
| :------- | :------: | :---------------------------- | :------------------------------------------------------------------------------------------------ |
| comments |    no    | array of strings              | One or more comment lines describing this rule.                                                   |
| id       |   yes    | string                        | Unique ID for this rule. Can only contain letters (A-Z, a-z), numbers (0-9), and underscore (\_). |
| actions  |   yes    | array of [actions](action.md) | One or more actions to execute.                                                                   |

## Return Value

Return value of the last action in the "actions" property.

## Example

```json
{
  "comments": ["Sets output voltage of PAGE 0 of a PMBus regulator"],
  "id": "set_page0_voltage_rule",
  "actions": [
    { "i2c_write_byte": { "register": "0x00", "value": "0x00" } },
    { "pmbus_write_vout_command": { "format": "linear" } }
  ]
}
```

... later in the config file ...

```json
{
  "configuration": {
    "volts": 1.03,
    "rule_id": "set_page0_voltage_rule"
  }
}
```
