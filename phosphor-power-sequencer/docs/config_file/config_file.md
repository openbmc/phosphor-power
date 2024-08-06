# config_file

## Description

The root (outer-most) object in the configuration file.

## Properties

| Name  | Required | Type                      | Description                                                                                                                 |
| :---- | :------: | :------------------------ | :-------------------------------------------------------------------------------------------------------------------------- |
| rails |   yes    | array of [rails](rail.md) | One or more voltage rails enabled or monitored by the power sequencer device. The rails must be in power on sequence order. |

## Example

```json
{
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
