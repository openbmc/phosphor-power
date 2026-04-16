# gpio

## Description

A General Purpose Input/Output (GPIO) that can be read to obtain the chassis
status.

GPIO values are read using the libgpiod interface.

## Properties

| Name      | Required | Type   | Description                                                                |
| --------- | -------- | ------ | -------------------------------------------------------------------------- |
| Name      | Yes      | string | The GPIO name as defined in the device tree                                |
| Direction | Yes      | string | Either "Input" or "Output"                                                 |
| Polarity  | Yes      | string | Either "Low" (active low) or "High" (active high)                          |
| Default   | No       | string | Optional default polarity for deglitching Polarity value. Either "Low" or  |
|           |          |        | "High". If not specified, the GPIO has no default value and getDefault()   |
|           |          |        | will return std::nullopt                                                   |

## Example

```json
"PresenceGpio": {
  "Name": "presence-chassis1",
  "Direction": "Input",
  "Polarity": "Low",
  "Default": "Low"
},
"FaultLatchResetGpio": {
    "Name": "power-chassis1-standby-fault-reset",
    "Direction": "Output",
    "Polarity": "Low"
}
```
