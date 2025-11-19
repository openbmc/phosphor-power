# power_sequencer

## Description

A power sequencer device within a chassis.

The power sequencer is responsible for performing the following tasks on a set
of [voltage rails](rail.md):

- Power the rails on in the correct order
- Monitor the rails for a [pgood fault](../pgood_faults.md)
- Power off the rails in the correct order

Note that some voltage rails may automatically power on/off without the power
sequencer, but they might still be monitored for pgood faults.

[Named GPIOs](../named_gpios.md) are used for the following:

- Power the sequencer on and off
- Monitor the power good signal from the sequencer. This signal is true when all
  rails are providing the expected voltage level.

## Properties

| Name                    |      Required       | Type                              | Description                                                                                                                   |
| :---------------------- | :-----------------: | :-------------------------------- | :---------------------------------------------------------------------------------------------------------------------------- |
| comments                |         no          | array of strings                  | One or more comment lines describing this power sequencer.                                                                    |
| type                    |         yes         | string                            | Power sequencer type. Specify one of the following supported types: "UCD90160", "UCD90320", "gpios_only_device".              |
| i2c_interface           | see [notes](#notes) | [i2c_interface](i2c_interface.md) | I2C interface to this power sequencer.                                                                                        |
| power_control_gpio_name |         yes         | string                            | Named GPIO for turning this power sequencer on and off.                                                                       |
| power_good_gpio_name    |         yes         | string                            | Named GPIO for reading the power good signal from this power sequencer.                                                       |
| rails                   | see [notes](#notes) | array of [rails](rail.md)         | One or more voltage rails powered on/off and monitored by this power sequencer. The rails must be in power on sequence order. |

### Notes

- The "i2c_interface" and "rails" properties are required if the "type" is
  "UCD90160" or "UCD90320". They are ignored if the "type" is
  "gpios_only_device".

## Example

```json
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
```
