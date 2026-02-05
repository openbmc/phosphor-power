# gpio

## Description

A General Purpose Input/Output (GPIO) that can be read to obtain the chassis
status.

GPIO values are read using the libgpiod interface.

## Data Format

The config file is a text file in the
[JSON (JavaScript Object Notation)](https://www.json.org/) data format.

## Properties

Each GPIO property (PresenceGpio, FaultUnlatchedGpio, etc.) containes GPIO
objects with the following properties:

- **Name**: The GPIO name as defined in the device tree
- **Direction**: Either "Input" or "Output"
- **Polarity**: Either "Low" (active low) or "High" (active high)

## Example

```json
{
  "Name": "Chassis 1 power gpios",
  "PresenceGpio": {
    "Name": "presence-chassis1",
    "Direction": "Input",
    "Polarity": "Low"
  },
  "FaultUnlatchedGpio": {
    "Name": "power-chassis1-standby-fault-unlatched",
    "Direction": "Input",
    "Polarity": "Low"
  }
}
```
