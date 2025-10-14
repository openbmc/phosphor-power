# chassis_template

## Description

Chassis templates are used to avoid duplicate data.

In a multiple chassis system, two or more of the [chassis](chassis.md) may have
the same hardware design. The corresponding chassis objects in the config file
would be almost identical with a few minor property value differences.

A chassis template defines the power sequencers, GPIOs, and voltage rails in a
chassis. One or more property values in the template contain variables, such as
`${chassis_number}`, to support chassis-specific values.

Multiple chassis can use the template rather than duplicating the information.
The individual chassis specify values for the template variables, such as
setting `${chassis_number}` to "1" or "2".

### Template Variables

Variables are specified in property values of a chassis template. This includes
the properties of contained sub-objects like
[power_sequencers](power_sequencer.md) or [rails](rail.md).

The syntax to specify a variable is `${variable_name}`.

Variable names can only contain letters (A-Z, a-z), numbers (0-9), and
underscores (\_).

Variables can be specified as the entire property value or as part of the
property value:

```json
"number": "${chassis_number}",
"power_good_gpio_name": "power-chassis${chassis_number}-good",
```

Variables are supported for the property types string, number, and boolean.

Variables must be specified within a string value (surrounded by double quotes).
If the property type is number or boolean, the string will be converted to the
required type.

## Properties

| Name             | Required | Type                                            | Description                                                                                                   |
| :--------------- | :------: | :---------------------------------------------- | :------------------------------------------------------------------------------------------------------------ |
| comments         |    no    | array of strings                                | One or more comment lines describing this chassis template.                                                   |
| id               |   yes    | string                                          | Unique ID for this chassis template. Can only contain letters (A-Z, a-z), numbers (0-9), and underscore (\_). |
| number           |   yes    | number                                          | Chassis number within the system. Chassis numbers start at 1 because chassis 0 represents the entire system.  |
| inventory_path   |   yes    | string                                          | D-Bus inventory path of the chassis, such as "/xyz/openbmc_project/inventory/system/chassis".                 |
| power_sequencers |   yes    | array of [power_sequencers](power_sequencer.md) | One or more power sequencer devices within the chassis.                                                       |

## Example

```json
{
  "id": "standard_chassis_template",
  "number": "${chassis_number}",
  "inventory_path": "/xyz/openbmc_project/inventory/system/chassis${chassis_number}",
  "power_sequencers": [
    {
      "type": "UCD90320",
      "i2c_interface": { "bus": "${sequencer_bus_number}", "address": "0x11" },
      "power_control_gpio_name": "power-chassis${chassis_number}-control",
      "power_good_gpio_name": "power-chassis${chassis_number}-good",
      "rails": [
        {
          "name": "VDD_CPU0",
          "page": 11,
          "check_status_vout": true
        },
        {
          "name": "VCS_CPU1",
          "presence": "/xyz/openbmc_project/inventory/system/chassis${chassis_number}/motherboard/cpu1",
          "gpio": { "line": 60 }
        }
      ]
    }
  ]
}
```
