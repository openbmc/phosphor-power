# device

## Description

A hardware device within the chassis.

The following devices should be defined in the config file:

- Voltage regulators that require configuration or monitoring.
- Other devices that are required to configure or monitor regulators. For
  example, an I/O expander may provide necessary information about a regulator.

## Properties

| Name                  | Required | Type                                              | Description                                                                                                                                                                                                                                                                                                                                    |
| :-------------------- | :------: | :------------------------------------------------ | :--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| comments              |    no    | array of strings                                  | One or more comment lines describing this device.                                                                                                                                                                                                                                                                                              |
| id                    |   yes    | string                                            | Unique ID for this device. Can only contain letters (A-Z, a-z), numbers (0-9), and underscore (\_).                                                                                                                                                                                                                                            |
| is_regulator          |   yes    | boolean (true or false)                           | Indicates whether this device is a voltage regulator (phase controller).                                                                                                                                                                                                                                                                       |
| fru                   |   yes    | string                                            | Field-Replaceable Unit (FRU) for this device. If the device itself is not a FRU, specify the FRU that contains it. Specify the relative D-Bus inventory path of the FRU. Full inventory paths begin with the root "/xyz/openbmc_project/inventory". Specify the relative path below the root, such as "system/chassis/motherboard/regulator2". |
| i2c_interface         |   yes    | [i2c_interface](i2c_interface.md)                 | I2C interface to this device.                                                                                                                                                                                                                                                                                                                  |
| presence_detection    |    no    | [presence_detection](presence_detection.md)       | Specifies how to detect whether this device is present. If this property is not specified, the device is assumed to always be present.                                                                                                                                                                                                         |
| configuration         |    no    | [configuration](configuration.md)                 | Specifies configuration changes that should be applied to this device. These changes usually override hardware default settings. The configuration changes are applied during the boot before regulators are enabled.                                                                                                                          |
| phase_fault_detection |    no    | [phase_fault_detection](phase_fault_detection.md) | Specifies how to detect and log redundant phase faults in this voltage regulator. Can only be specified if the "is_regulator" property is true.                                                                                                                                                                                                |
| rails                 |    no    | array of [rails](rail.md)                         | One or more voltage rails produced by this device. Can only be specified if the "is_regulator" property is true.                                                                                                                                                                                                                               |

## Example

```json
{
  "comments": [ "IR35221 regulator producing the Vdd rail" ],
  "id": "vdd_regulator",
  "is_regulator": true,
  "fru": "system/chassis/motherboard/regulator2",
  "i2c_interface": {
    "bus": 1,
    "address": "0x70"
  },
  "configuration": {
    "rule_id": "configure_ir35221_rule"
  },
  "rails": [
    {
      "id": "vdd",
      ... details omitted ...
    }
  ]
}
```
