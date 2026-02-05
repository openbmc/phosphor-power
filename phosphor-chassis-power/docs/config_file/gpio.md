# gpio

## Description

A General Purpose Input/Output (GPIO) that can be read to obtain the chassis
status.

GPIO values are read using the libgpiod interface.

## Properties

| Name                  | Required | Type   | Description                                       |
| --------------------- | -------- | ------ | ------------------------------------------------- |
| Name                  | Yes      | string | The GPIO name as defined in the device tree       |
| Direction             | Yes      | string | Either "Input" or "Output"                        |
| Direction             | Yes      | string | Either "Low" (active low) or "High" (active high) |

## Example

```json
"PresenceGpio": {
  "Name": "presence-chassis1",
  "Direction": "Input",
  "Polarity": "Low"
}
```
