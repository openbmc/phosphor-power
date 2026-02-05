# phosphor-chassis-power Configuration File

## Table of Contents

- [Overview](#overview)
- [Location](#location)
- [Default Configuration File](#default-configuration-file)
- [System-Specific Configuration Files](#system-specific-configuration-files)
- [Selecting Configuration File To Use](#selecting-configuration-file-to-use)
- [Data Format](#data-format)
- [Contents](#contents)

## Overview

The `phosphor-chassis-power` application is controlled by a configuration file
(config file). The config file contains information about the chassis, power
GPIOs.

This information is used to detect Presence, Faults, and System Reset GPIOs.

## Location

Config files are stored in the [config_files](../../config_files) directory of
this repository.

The config files are installed in the `/usr/share/phosphor-chassis-power`
directory on the BMC.

## Default Configuration File

There is no default configuration, since GPIO configuration is needed for
functionality.

## System-Specific Configuration Files

Config files are often system-specific. Each system type usually has a different
set of chassis, GPIOs.

## Selecting Configuration File To Use

The meson build option `sequencer-use-default-config-file` determines which
config file is used: SHELDON:Question: would this be `chassis-power-use-config-file`

See [meson.options](../../../meson.options).

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

A schema detailing the hardware used in a chassis to monitor and interact with
power supplies currently using GPIOs.

The [chassis_power_monitor.json](../../config_files/chassis_power_monitor.json)
schema file defines a JSON schema for "ChassisPowerMonitor" with properties for
various GPIO configurations and chassis identification used for monitoring
chassis presence and faults.

## Properties

| Name                  | Required | Type   | Description                                                                                                                       |
| --------------------- | -------- | ------ | --------------------------------------------------------------------------------------------------------------------------------- |
| Name                  | Yes      | string | The name of this chassis power monitor configuration                                                                              |
| Type                  | Yes      | enum   | Must be "ChassisPowerMonitor"                                                                                                     |
| ChassisID             | Yes      | number | The ID of the chassis these GPIOs are for                                                                                         |
| PresenceGpio          | No       | Gpio   | The GPIO to monitor that indicates whether this chassis is present                                                                |
| FaultUnlatchedGpio    | No       | Gpio   | The GPIO to monitor that indicates the current (live) state of the input power of this chassis                                    |
| FaultLatchedGpio      | No       | Gpio   | The GPIO to monitor that indicates if an input power fault was latched for this chassis (eg between reads of unlatched gpio)      |
| FaultLatchResetGpio   | No       | Gpio   | The GPIO to disable (reset) the latched input power loss gpio for this chassis                                                    |
| EnableSystemResetGpio | No       | Gpio   | The GPIO to enable a full system reset if/when this chassis has a loss of input power                                             |
| FSILinkCheck          | No       | string | The device path to check existence of to verify the fsi link to this chassis is working. Eg UCD i2c bus                           |

## GPIO Configuration

Each GPIO property (PresenceGpio, FaultUnlatchedGpio, etc.) is an array containing GPIO objects with the following properties:

- **Name**: The GPIO name as defined in the device tree
- **Direction**: Either "Input" or "Output"
- **Polarity**: Either "Low" (active low) or "High" (active high)
- **Type**: Must be "Gpio"

### Comments

The JSON data format does not support comments. However, some of the JSON
objects in the config file provide an optional "comments" property. The value of
this property is an array of strings. Use this property to annotate the config
file. The "comments" properties are ignored when the config file is read by the
`phosphor-chassis-power` application.

Examples:

```json
"comments": ["Chassis 2: Standard hardware layout"],
```

```json
"comments": ["The first of two UCD90320 power sequencers in the chassis.",
             "Controls/monitors rails 0-15."]
```

### Hexadecimal Values

The JSON number data type does not support the hexadecimal format. For this
reason, properties with hexadecimal values use the string data type.

Example:

```json
"address": "0x70"
```

[1]:
  https://github.com/openbmc/phosphor-dbus-interfaces/blob/master/yaml/xyz/openbmc_project/Inventory/Decorator/Compatible.interface.yaml
