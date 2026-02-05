# phosphor-chassis-power Configuration File

## Table of Contents

- [Overview](#overview)
- [Location](#location)
- [System-Specific Configuration Files](#system-specific-configuration-files)
- [Selecting Configuration File To Use](#selecting-configuration-file-to-use)
- [Data Format](#data-format)
- [Contents](#contents)
- [Example](#example)

## Overview

The `phosphor-chassis-power` application is controlled by a configuration file
(config file). The config file contains information about the chassis power
GPIOs.

This information is used to detect Presence, Faults, and System Reset GPIOs.

## Location

Config files are stored in the [config_files](../../config_files) directory of
this repository.

The config files are installed in the `/usr/share/phosphor-chassis-power`
directory on the BMC.

## System-Specific Configuration Files

Config files are often system-specific. Each system type usually has a different
set of chassis GPIOs.

## Selecting Configuration File To Use

System-specific config files are found using the
[`xyz.openbmc_project.Inventory.Decorator.Compatible`][1] D-Bus interface.

The `phosphor-chassis-power` application searches for a D-Bus Chassis object
created by [Entity Manager](https://github.com/openbmc/entity-manager) that
implements the `Compatible` interface.

The `Names` property of this interface contains a list of one or more compatible
system types. The types are ordered from most specific to least specific.

Example:

- `com.acme.Hardware.Chassis.Model.MegaServer4CPU`
- `com.acme.Hardware.Chassis.Model.MegaServer`
- `com.acme.Hardware.Chassis.Model.Server`

The application searches for a config file name that matches one of these
compatible system types. It searches from most specific to least specific. The
first config file found will be used.

For each compatible system type, the application will look for two config file
names:

- The complete compatible system type plus a '.json' suffix
- The last node of the compatible system type plus a '.json' suffix

Example:

- `com.acme.Hardware.Chassis.Model.MegaServer4CPU.json`
- `MegaServer4CPU.json`

## Data Format

The config file is a text file in the
[JSON (JavaScript Object Notation)](https://www.json.org/) data format.

## Contents

The JSON file will have an overall format of an array for the system with 1 to N
chassis. The Chassis will have the format outlined in [chassis](chassis.md).

## Example

```json
[
  {
    "ChassisNumber": 1,
    "PresenceGpio": {
        "Name": "presence-chassis1",
        "Direction": "Input",
        "Polarity": "Low"
    }
  },
  {
    "ChassisNumber": 2,
    "PresenceGpio": {
        "Name": "presence-chassis2",
        "Direction": "Input",
        "Polarity": "Low"
    }
  }
]
```

[1]:
  https://github.com/openbmc/phosphor-dbus-interfaces/blob/master/yaml/xyz/openbmc_project/Inventory/Decorator/Compatible.interface.yaml
