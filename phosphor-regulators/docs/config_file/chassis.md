# chassis

## Description

A chassis within the system.

Chassis are typically a physical enclosure that contains system components such
as CPUs, fans, power supplies, and PCIe cards. A chassis can be stand-alone,
such as a tower or desktop. A chassis can also be designed to be mounted in an
equipment rack.

A chassis only needs to be defined in the config file if it contains regulators
that need to be configured or monitored.

### Chassis Templates

In a multiple chassis system, two or more of the chassis may have the same
hardware design.

A [chassis template](chassis_template.md) can be used to avoid duplicate data.

Specify the "template_id" and "template_variable_values" properties to use a
chassis template.

## Properties

| Name                     |      Required       | Type                                                    | Description                                                                                                                                                                                              |
| :----------------------- | :-----------------: | :------------------------------------------------------ | :------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| comments                 |         no          | array of strings                                        | One or more comment lines describing this chassis.                                                                                                                                                       |
| number                   | see [notes](#notes) | number                                                  | Chassis number within the system. Chassis numbers start at 1 because chassis 0 represents the entire system.                                                                                             |
| inventory_path           | see [notes](#notes) | string                                                  | Specify the relative D-Bus inventory path of the chassis. Full inventory paths begin with the root "/xyz/openbmc_project/inventory". Specify the relative path below the root, such as "system/chassis". |
| devices                  | see [notes](#notes) | array of [devices](device.md)                           | One or more devices within the chassis. The array should contain regulator devices and any related devices required to perform regulator operations.                                                     |
| status_monitoring        |         no          | [status_monitoring](status_monitoring.md)               | Status monitoring to perform on this chassis, such as verifying it is present.                                                                                                                           |
| template_id              | see [notes](#notes) | string                                                  | Unique ID of the [chassis template](chassis_template.md) to use for the contents of this chassis.                                                                                                        |
| template_variable_values | see [notes](#notes) | [template_variable_values](template_variable_values.md) | Chassis-specific values for chassis template variables.                                                                                                                                                  |

### Notes

- You must specify exactly one of the following two groups of properties:
  - "number", "inventory_path", and optionally "devices"
  - "template_id" and "template_variable_values"
- "comments" is optional and can be specified with either property group
- "status_monitoring" is optional
  - If specified, it should be in the same property group as "number",
    "inventory_path", and "devices"
  - If not specified, the chassis status will be assumed good (present,
    available, enabled, etc.)

## Examples

```json
{
  "comments": [ "Chassis number 1 containing CPUs and memory" ],
  "number": 1,
  "inventory_path": "system/chassis",
  "devices": [
    {
      "id": "vdd_regulator",
      ... details omitted ...
    },
    {
      "id": "vio_regulator",
      ... details omitted ...
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
    "vdd_regulator_bus": "7"
  }
}
```
