# rail

## Description

A voltage rail produced by a regulator.

Voltage regulators produce one or more rails. Each rail typically provides a
different output voltage level, such as 1.1V.

On a PMBus regulator with multiple rails, the current rail is selected using the
PAGE command. Subsequent PMBus commands are sent to that PAGE/rail.

## Properties

| Name              | Required | Type                                      | Description                                                                                                                                                                                                         |
| :---------------- | :------: | :---------------------------------------- | :------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------ |
| comments          |    no    | array of strings                          | One or more comment lines describing this rail.                                                                                                                                                                     |
| id                |   yes    | string                                    | Unique ID for this rail. Can only contain letters (A-Z, a-z), numbers (0-9), and underscore (\_).                                                                                                                   |
| configuration     |    no    | [configuration](configuration.md)         | Specifies configuration changes that should be applied to this rail. These changes usually override hardware default settings. The configuration changes are applied during the boot before regulators are enabled. |
| sensor_monitoring |    no    | [sensor_monitoring](sensor_monitoring.md) | Specifies how to read the sensors for this rail.                                                                                                                                                                    |

## Example

```json
{
  "comments": ["Vdd rail on PAGE 0 of the Vdd/Vio regulator"],
  "id": "vdd",
  "configuration": {
    "volts": 1.1,
    "rule_id": "set_page0_voltage_rule"
  },
  "sensor_monitoring": {
    "rule_id": "read_ir35221_page0_sensors_rule"
  }
}
```
