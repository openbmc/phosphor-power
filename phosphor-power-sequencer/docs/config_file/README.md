# phosphor-power-sequencer Configuration File

## Table of Contents

- [Overview](#overview)
- [Location](#location)
- [Default Configuration File](#default-configuration-file)
- [System-Specific Configuration Files](#system-specific-configuration-files)
- [Selecting Configuration File To Use](#selecting-configuration-file-to-use)
- [Data Format](#data-format)
- [Contents](#contents)

## Overview

The `phosphor-power-sequencer` application is controlled by a configuration file
(config file). The config file contains information about the chassis, power
sequencer devices, GPIOs, and voltage rails within the system.

This information is used to power the system on/off, detect
[pgood faults](../pgood_faults.md), and identify the voltage rail that caused a
pgood fault.

## Location

Config files are stored in the [config_files](../../config_files) directory of
this repository.

The config files are installed in the `/usr/share/phosphor-power-sequencer`
directory on the BMC.

## Default Configuration File

The default config file is [Default.json](../../config_files/Default.json). This
simple file contains one chassis, one power sequencer device, and no voltage
rails.

the default config file can be used if the following are all true:

- The system uses the default names for the power control and power good GPIOs.
- No isolation is required to determine which rail caused a power good fault.

## System-Specific Configuration Files

Config files are often system-specific. Each system type usually has a different
set of chassis, power sequencer devices, GPIOs, and voltage rails.

System-specific config files can support isolating which rail caused a power
good fault.

## Selecting Configuration File To Use

The meson build option `sequencer-use-default-config-file` determines which
config file is used:

- If the option is true, the default config file is used.
- If the option is false, a system-specific config file is used.

The option is true by default. See [meson.options](../../../meson.options).

System-specific config files are found using the
[`xyz.openbmc_project.Inventory.Decorator.Compatible`][1] D-Bus interface.

The `phosphor-power-sequencer` application searches for a D-Bus Chassis object
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

[1]:
  https://github.com/openbmc/phosphor-dbus-interfaces/blob/master/yaml/xyz/openbmc_project/Inventory/Decorator/Compatible.interface.yaml
