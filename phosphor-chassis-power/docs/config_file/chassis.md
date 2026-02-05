# chassis

## Description

A chassis within the system.

Chassis are typically a physical enclosure that contains system components such
as CPUs, fans, power supplies, and PCIe cards. A chassis can be stand-alone,
such as a tower or desktop. A chassis can also be designed to be mounted in an
equipment rack.

## Properties

| Name                  | Required | Type   | Description                                                                                                                  |
| --------------------- | -------- | ------ | ---------------------------------------------------------------------------------------------------------------------------- |
| ChassisNumber         | Yes      | number | The number of the chassis these GPIOs are for                                                                                |
| PresenceGpio          | No       | [GPIO](gpio.md)   | GPIO that indicates whether chassis is present.                                                                              |
| FaultUnlatchedGpio    | No       | [GPIO](gpio.md)   | The GPIO to monitor that indicates the current (live) state of the input power of this chassis                               |
| FaultLatchedGpio      | No       | [GPIO](gpio.md)   | The GPIO to monitor that indicates if an input power fault was latched for this chassis (eg between reads of unlatched gpio) |
| FaultLatchResetGpio   | No       | [GPIO](gpio.md)   | The GPIO to reset the latched input power loss gpio for this chassis                                                         |
| EnableSystemResetGpio | No       | [GPIO](gpio.md)   | The GPIO to enable a full system reset if/when this chassis has a loss of input power                                        |
| PresencePath          | No       | [GPIO](gpio.md)   | File path that indicates whether chassis is present. If file exists, chassis is present                                      |

## Chassis Presence

If the PresenceGpio property is specified, the GPIO will be read. If the GPIO is
active, such as a 1 value with Polarity High, then the chassis is present. If
the PresencePath property is specified, then if the specified path exists the
chassis is present. If both properties are specified, the chassis is present if
either property indicates it is present

## Examples

```json
[
  {
    "ChassisID": 1,
    "PresenceGpio": {},
    "FaultUnlatchedGpio": {},
    "FaultLatchedGpio": {},
    "FaultLatchResetGpio": {},
    "EnableSystemResetGpio": {},
    "PresencePath": ". . ."
  },
  {
    "ChassisID": 2,
    "PresenceGpio": {},
    "FaultUnlatchedGpio": {},
    "FaultLatchedGpio": {},
    "FaultLatchResetGpio": {},
    "EnableSystemResetGpio": {},
    "PresencePath": ". . ."
  }
]
```
