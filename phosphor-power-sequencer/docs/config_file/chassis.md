# chassis

## Description

A chassis within the system.

Chassis are typically a physical enclosure that contains system components such
as CPUs, fans, power supplies, and PCIe cards. A chassis can be stand-alone,
such as a tower or desktop. A chassis can also be designed to be mounted in an
equipment rack.

A chassis only needs to be defined in the config file if it contains a power
sequencer device.

### Chassis Templates

In a multiple chassis system, two or more of the chassis may have the same
hardware design.

A [chassis template](chassis_template.md) can be used to avoid duplicate data.

Specify the "template_id" and "template_variable_values" properties to use a
chassis template.

## Properties

| Name                     |      Required       | Type                                                    | Description                                                                                                  |
| :----------------------- | :-----------------: | :------------------------------------------------------ | :----------------------------------------------------------------------------------------------------------- |
| comments                 |         no          | array of strings                                        | One or more comment lines describing this chassis.                                                           |
| number                   | see [notes](#notes) | number                                                  | Chassis number within the system. Chassis numbers start at 1 because chassis 0 represents the entire system. |
| inventory_path           | see [notes](#notes) | string                                                  | D-Bus inventory path of the chassis, such as "/xyz/openbmc_project/inventory/system/chassis".                |
| power_sequencers         | see [notes](#notes) | array of [power_sequencers](power_sequencer.md)         | One or more power sequencer devices within the chassis.                                                      |
| template_id              | see [notes](#notes) | string                                                  | Unique ID of the [chassis template](chassis_template.md) to use for the contents of this chassis.            |
| template_variable_values | see [notes](#notes) | [template_variable_values](template_variable_values.md) | Chassis-specific values for chassis template variables.                                                      |

### Notes

- You must specify exactly one of the following two groups of properties:
  - "number", "inventory_path", and "power_sequencers"
  - "template_id" and "template_variable_values"

## Examples

```json
{
  "number": 1,
  "inventory_path": "/xyz/openbmc_project/inventory/system/chassis",
  "power_sequencers": [
    {
      "type": "UCD90320",
      "i2c_interface": { "bus": 3, "address": "0x11" },
      "power_control_gpio_name": "power-chassis-control",
      "power_good_gpio_name": "power-chassis-good",
      "rails": [
        {
          "name": "VDD_CPU0",
          "page": 11,
          "check_status_vout": true
        },
        {
          "name": "VCS_CPU1",
          "presence": "/xyz/openbmc_project/inventory/system/chassis/motherboard/cpu1",
          "gpio": { "line": 60 }
        }
      ]
    }
  ]
}
```

```json
{
  "comments": ["Chassis 2: Standard hardware layout"],
  "template_id": "standard_chassis_template",
  "template_variable_values": {
    "chassis_number": "2",
    "sequencer_bus_number": "13"
  }
}
```
