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
| Name                  | Yes      | string | The name of this chassis power monitor configuration                                                                         |
| ChassisID             | Yes      | number | The ID of the chassis these GPIOs are for                                                                                    |
| PresenceGpio          | No       | Gpio   | GPIO that indicates whether chassis is present.                                                                              |
| FaultUnlatchedGpio    | No       | Gpio   | The GPIO to monitor that indicates the current (live) state of the input power of this chassis                               |
| FaultLatchedGpio      | No       | Gpio   | The GPIO to monitor that indicates if an input power fault was latched for this chassis (eg between reads of unlatched gpio) |
| FaultLatchResetGpio   | No       | Gpio   | The GPIO to disable (reset) the latched input power loss gpio for this chassis                                               |
| EnableSystemResetGpio | No       | Gpio   | The GPIO to enable a full system reset if/when this chassis has a loss of input power                                        |
| PresencePath          | No       | string | File path that indicates whether chassis is present. If file exists, chassis is present                                      |

## Chassis Presence

If the PresenceGpio is detected this indicates the chassis is present.  If the
PresenceGpio if not detected but the PresencePath is present, this indicates the
chassis is present.  If both PresenceGpio and PresencePath are not detected, the
chassis is not present.

## Data Format

The config file is a text file in the
[JSON (JavaScript Object Notation)](https://www.json.org/) data format.

## GPIO Contents

Each chassis can have multiple GPIO entries with the format outlined [GPIO format](gpio.md).

## Examples

```json
[
    { "Name": "Chassis 1 power gpios",
        "PresenceGpio":{},
        "FaultUnlatchedGpio":{},
        "FaultLatchedGpio":{},
        "FaultLatchResetGpio":{},
        "EnableSystemResetGpio":{},
        "PresencePath": ". . .",
        "ChassisID": 1
    },
    { "Name": "Chassis 1 power gpios",
            "PresenceGpio":{},
        "FaultUnlatchedGpio":{},
        "FaultLatchedGpio":{},
        "FaultLatchResetGpio":{},
        "EnableSystemResetGpio":{},
        "PresencePath": ". . .",
        "ChassisID": 2
    }
]
```
