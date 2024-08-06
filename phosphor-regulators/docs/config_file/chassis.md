# chassis

## Description

A chassis within the system.

Chassis are large enclosures that can be independently powered off and on by the
BMC. Small and mid-sized systems may contain a single chassis. In a large
rack-mounted system, each drawer may correspond to a chassis.

A chassis only needs to be defined in the config file if it contains regulators
that need to be configured or monitored.

## Properties

| Name           | Required | Type                          | Description                                                                                                                                                                                              |
| :------------- | :------: | :---------------------------- | :------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| comments       |    no    | array of strings              | One or more comment lines describing this chassis.                                                                                                                                                       |
| number         |   yes    | number                        | Chassis number within the system. Chassis numbers start at 1 because chassis 0 represents the entire system.                                                                                             |
| inventory_path |   yes    | string                        | Specify the relative D-Bus inventory path of the chassis. Full inventory paths begin with the root "/xyz/openbmc_project/inventory". Specify the relative path below the root, such as "system/chassis". |
| devices        |    no    | array of [devices](device.md) | One or more devices within the chassis. The array should contain regulator devices and any related devices required to perform regulator operations.                                                     |

## Example

```json
{
  "comments": [ "Chassis number 1 containing CPUs and memory" ],
  "number": 1,
  "inventory_path": "system/chassis",
  "devices": [
    {
      "id": "vdd_regulator",
      ... details omitted ...
    },
    {
      "id": "vio_regulator",
      ... details omitted ...
    }
  ]
}
```
