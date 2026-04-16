# gpio

## Description

A General Purpose Input/Output (GPIO) that can be read to obtain the chassis
status.

GPIO values are read using the libgpiod interface.

## Properties

| Name         | Required | Type   | Description                                                           |
| ------------ | -------- | ------ | --------------------------------------------------------------------- |
| Name         | Yes      | string | The GPIO name as defined in the device tree                           |
| Direction.   | Yes      | string | Either "Input" or "Output"                                            |
| Polarity     | Yes      | string | Either "Low" (active low) or "High" (active high)                     |
| defaultValue | no       | number | Optional default value for deglitching reading value. Either 0 or 1.  |
|              |          |        | If not specified, the GPIO has no default value and getDefault() will |
|              |          |        | return std::nullopt                                                   |

## Example

```json
"PresenceGpio": {
  "Name": "presence-chassis1",
  "Direction": "Input",
  "Polarity": "Low",
  "Default": 0
},
"FaultLatchResetGpio": {
    "Name": "power-chassis1-standby-fault-reset",
    "Direction": "Output",
    "Polarity": "Low"
}
```
