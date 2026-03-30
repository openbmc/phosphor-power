# config_file

## Description

The root (outer-most) object in a phosphor-regulators configuration file.

## Properties

| Name              | Required | Type                                              | Description                                                                                            |
| :---------------- | :------: | :------------------------------------------------ | :----------------------------------------------------------------------------------------------------- |
| comments          |    no    | array of strings                                  | One or more comment lines describing this file.                                                        |
| rules             |    no    | array of [rules](rule.md)                         | One or more rules shared by regulators in this file.                                                   |
| chassis_templates |    no    | array of [chassis_templates](chassis_template.md) | One or more chassis templates. Templates are used to avoid duplicate data in multiple chassis systems. |
| chassis           |   yes    | array of [chassis](chassis.md)                    | One or more chassis in the system.                                                                     |

## Examples

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

```json
{
  "comments": [ "Config file for a multi-chassis system" ],
  "rules": [
    {
      "id": "set_voltage_rule",
      ... details omitted ...
    }
  ],
  "chassis_templates": [
    {
      "id": "standard_chassis_template",
      "number": "${chassis_number}",
      "inventory_path": "system/chassis${chassis_number}",
      "devices": [
        ... details omitted ...
      ]
    }
  ],
  "chassis": [
    {
      "comments": [ "Chassis 1" ],
      "template_id": "standard_chassis_template",
      "template_variable_values": {
        "chassis_number": "1"
      }
    },
    {
      "comments": [ "Chassis 2" ],
      "template_id": "standard_chassis_template",
      "template_variable_values": {
        "chassis_number": "2"
      }
    }
  ]
}
```
