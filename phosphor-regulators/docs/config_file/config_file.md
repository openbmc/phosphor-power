# config_file

## Description

The root (outer-most) object in a phosphor-regulators configuration file.

## Properties

| Name     | Required | Type                           | Description                                          |
| :------- | :------: | :----------------------------- | :--------------------------------------------------- |
| comments |    no    | array of strings               | One or more comment lines describing this file.      |
| rules    |    no    | array of [rules](rule.md)      | One or more rules shared by regulators in this file. |
| chassis  |   yes    | array of [chassis](chassis.md) | One or more chassis in the system.                   |

## Example

```json
{
  "comments": [ "Config file for a FooBar one-chassis system" ],
  "rules": [
    {
      "id": "set_voltage_rule",
      ... details omitted ...
    },
    {
      "id": "read_sensors_rule",
      ... details omitted ...
    }
  ],
  "chassis": [
    {
      "number": 1,
      "inventory_path": "system/chassis",
      "devices": [
        ... details omitted ...
      ]
    }
  ]
}
```
