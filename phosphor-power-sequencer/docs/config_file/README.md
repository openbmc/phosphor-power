# phosphor-power-sequencer Configuration File

## Table of Contents

- [Overview](#overview)
- [Data Format](#data-format)
- [Name](#name)
- [Contents](#contents)
- [Installation](#installation)

## Overview

The `phosphor-power-sequencer` application is controlled by a configuration file
(config file). The config file contains information about the chassis, power
sequencer devices, GPIOs, and voltage rails within the system.

This information is used to power the system on/off, detect
[pgood faults](../pgood_faults.md), and identify the voltage rail that caused
for a pgood fault.

The config file is optional. If no file is found, the application will do the
following:

- Assume this is a single chassis system.
- Assume the standard [GPIO names](../named_gpios.md) are used to power the
  system on/off and detect pgood faults.
- Log a general error if a pgood fault occurs. No specific voltage rail will be
  identified.

## Data Format

The config file is a text file in the
[JSON (JavaScript Object Notation)](https://www.json.org/) data format.

## Name

The config file name is based on the system type that it supports.

A config file is normally system-specific. Each system type usually has a
different set of chassis, power sequencer devices, GPIOs, and voltage rails.

The system type is obtained from a D-Bus Chassis object created by the
[Entity Manager](https://github.com/openbmc/entity-manager) application. The
object must implement the `xyz.openbmc_project.Inventory.Decorator.Compatible`
interface.

The `Names` property of this interface contains a list of one or more compatible
system types. The types are ordered from most specific to least specific.

Example:

- `com.acme.Hardware.Chassis.Model.MegaServer4CPU`
- `com.acme.Hardware.Chassis.Model.MegaServer`
- `com.acme.Hardware.Chassis.Model.Server`

The `phosphor-power-sequencer` application searches for a config file name that
matches one of these compatible system types. It searches from most specific to
least specific. The first config file found, if any, will be used.

For each compatible system type, the application will look for two config file
names:

- The complete compatible system type plus a '.json' suffix
- The last node of the compatible system type plus a '.json' suffix

Example:

- `com.acme.Hardware.Chassis.Model.MegaServer4CPU.json`
- `MegaServer4CPU.json`

## Contents

The config file contains a single JSON [config_file](config_file.md) object at
the root level. That object contains arrays of other JSON objects.

### Comments

The JSON data format does not support comments. However, some of the JSON
objects in the config file provide an optional "comments" property. The value of
this property is an array of strings. Use this property to annotate the config
file. The "comments" properties are ignored when the config file is read by the
`phosphor-power-sequencer` application.

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

## Installation

The config file is installed in the `/usr/share/phosphor-power-sequencer`
directory on the BMC.
