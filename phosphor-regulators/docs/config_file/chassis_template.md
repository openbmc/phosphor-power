# chassis_template

## Description

Chassis templates are used to avoid duplicate data.

In a multiple chassis system, two or more of the [chassis](chassis.md) may have
the same hardware design. The corresponding chassis objects in the config file
would be almost identical with a few minor property value differences.

A chassis template defines the regulator devices and voltage rails in a chassis.
One or more property values in the template contain variables, such as
`${chassis_number}`, to support chassis-specific values.

Multiple chassis can use the template rather than duplicating the information.
The individual chassis specify values for the template variables, such as
setting `${chassis_number}` to "1" or "2".

### Template Variables

Variables are specified in property values of a chassis template. This includes
the properties of contained sub-objects like [devices](device.md) or
[rails](rail.md).

The syntax to specify a variable is `${variable_name}`.

Variable names can only contain letters (A-Z, a-z), numbers (0-9), and
underscores (\_).

Variables can be specified as the entire property value or as part of the
property value:

```json
"number": "${chassis_number}",
"fru": "system/chassis${chassis_number}/motherboard/regulator1",
```

Variables are supported for the property types string, number, and boolean.

Variables must be specified within a string value (surrounded by double quotes).
If the property type is number or boolean, the string will be converted to the
required type.

Variables are **not** allowed within [rules](rule.md) or the actions they
contain.

## Properties

| Name           | Required | Type                          | Description                                                                                                                                                                                                               |
| :------------- | :------: | :---------------------------- | :------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------ |
| comments       |    no    | array of strings              | One or more comment lines describing this chassis template.                                                                                                                                                               |
| id             |   yes    | string                        | Unique ID for this chassis template. Can only contain letters (A-Z, a-z), numbers (0-9), and underscore (\_).                                                                                                             |
| number         |   yes    | number                        | Chassis number within the system. Chassis numbers start at 1 because chassis 0 represents the entire system.                                                                                                              |
| inventory_path |   yes    | string                        | Specify the relative D-Bus inventory path of the chassis. Full inventory paths begin with the root "/xyz/openbmc_project/inventory". Specify the relative path below the root, such as "system/chassis${chassis_number}". |
| devices        |    no    | array of [devices](device.md) | One or more devices within the chassis. The array should contain regulator devices and any related devices required to perform regulator operations.                                                                      |

## Example

```json
{
  "comments": ["Template for a chassis in a multi-chassis system"],
  "id": "multi_chassis_template",
  "number": "${chassis_number}",
  "inventory_path": "system/chassis${chassis_number}",
  "devices": [
    {
      "id": "chassis${chassis_number}_vdd_regulator",
      "is_regulator": true,
      "fru": "system/chassis${chassis_number}/motherboard/regulator1",
      "i2c_interface": {
        "bus": "${vdd_regulator_bus}",
        "address": "0x70"
      },
      "rails": [
        {
          "id": "chassis${chassis_number}_vdd",
          "configuration": {
            "volts": 1.1,
            "rule_id": "set_voltage_rule"
          },
          "sensor_monitoring": {
            "rule_id": "read_sensors_rule"
          }
        }
      ]
    }
  ]
}
```
