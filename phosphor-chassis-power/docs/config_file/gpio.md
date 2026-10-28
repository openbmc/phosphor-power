# gpio

## Description

A General Purpose Input/Output (GPIO) that can be read to obtain the chassis
status.

GPIO values are read using the libgpiod interface.

## Properties

| Name         | Required | Type   | Description                                                                                                                         |
| ------------ | -------- | ------ | ----------------------------------------------------------------------------------------------------------------------------------- |
| Name         | Yes      | string | The GPIO name as defined in the device tree                                                                                         |
| Direction    | Yes      | string | Either "Input" or "Output"                                                                                                          |
| Polarity     | Yes      | string | Either "Low" (active low) or "High" (active high)                                                                                   |
| DefaultValue | no       | number | Default GPIO value (0 or 1). Only used if the first attempt to read GPIO fails. Only valid for GPIOs with Direction set to "Input". |
| DevicePath   | no       | string | Absolute sysfs path of the device providing the GPIO. Used to call out the device in the error log if a GPIO error occurs.          |

## Example

```json
"PresenceGpio": {
  "Name": "presence-chassis1",
  "Direction": "Input",
  "Polarity": "Low",
  "DefaultValue": 0
},
"FaultLatchResetGpio": {
    "Name": "power-chassis1-standby-fault-reset",
    "Direction": "Output",
    "Polarity": "Low",
    "DevicePath": "/sys/bus/i2c/devices/9-0073"
}
```
