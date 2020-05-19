# phosphor-regulators Configuration File

## Overview

The `phosphor-regulators` application is controlled by a configuration file
(config file).  The config file defines how to perform the following operations
on voltage regulators in the system:
* Modify regulator configuration, such as output voltage or overcurrent
  settings
* Read sensor values

The config file does not control how voltage regulators are enabled or how to
monitor their Power Good (pgood) status.  Those operations are typically
performed by power sequencer hardware and related firmware.


## Data Format

The config file is written using the [JSON (JavaScript Object
Notation)](https://www.json.org/) data format.

The config file can be created using any text editor, such as Atom, Notepad++,
gedit, Vim, or Emacs.  You can select any file name, but it must end with the
".json" suffix, such as `regulators.json`.


## Example

See [config_file.json](../../examples/config_file.json).


## Contents

### Structure

The config file typically contains the following structure:
* Array of [rules](rule.md)
  * Rules defining how to modify configuration of regulators
  * Rules defining how to read sensors
* Array of [chassis](chassis.md) in the system
  * Array of regulator [devices](device.md) in the chassis
    * Array of voltage [rails](rail.md) produced by the regulator

Rules provide common, reusable sequences of actions that can be shared by one
or more regulators.  They are optional and can be omitted if each regulator
requires a unique sequence of actions.

### Syntax

The config file contains a single JSON [config_file](config_file.md) object at
the root level.  That object contains arrays of other JSON objects.

The following JSON object types are supported:
* [action](action.md)
* [and](and.md)
* [chassis](chassis.md)
* [compare_presence](compare_presence.md)
* [compare_vpd](compare_vpd.md)
* [config_file](config_file.md)
* [configuration](configuration.md)
* [device](device.md)
* [i2c_compare_bit](i2c_compare_bit.md)
* [i2c_compare_byte](i2c_compare_byte.md)
* [i2c_compare_bytes](i2c_compare_bytes.md)
* [i2c_interface](i2c_interface.md)
* [i2c_write_bit](i2c_write_bit.md)
* [i2c_write_byte](i2c_write_byte.md)
* [i2c_write_bytes](i2c_write_bytes.md)
* [if](if.md)
* [not](not.md)
* [or](or.md)
* [pmbus_read_sensor](pmbus_read_sensor.md)
* [pmbus_write_vout_command](pmbus_write_vout_command.md)
* [presence_detection](presence_detection.md)
* [rail](rail.md)
* [rule](rule.md)
* [run_rule](run_rule.md)
* [sensor_monitoring](sensor_monitoring.md)
* [set_device](set_device.md)

### Comments

The JSON data format does not support comments.  However, many of the JSON
objects in the config file provide an optional "comments" property.  The value
of this property is an array of strings.  Use this property to annotate the
config file.  The "comments" properties are ignored when the config file is
read by the `phosphor-regulators` application.

Examples:

```
"comments": [ "IR35221 regulator producing the Vdd rail" ]

"comments": [ "Check if register 0x82 contains 0x7302",
              "Device returns bytes in little-endian order",
              "Ignore most significant bit in each byte" ]
```

### Hexadecimal Values

The JSON number data type does not support the hexadecimal format.  For this
reason, properties with hexadecimal values use the string data type.

Example:
```
"address": "0x70"
```


## Validation

After creating or modifying a config file, you need to validate it using the
tool [validate-regulators-config.py](../../tools/validate-regulators-config.py).

The validation tool checks the config file for errors, such as:
* Invalid JSON syntax (like a missing brace)
* Unrecognized JSON object or property
* Duplicate rule or device ID
* Reference to a rule or device ID that does not exist
* Infinite loop, such as rule A runs rule B which runs rule A

The tool requires two input files:
* config file to validate
* [config_schema.json](../../schema/config_schema.json)

The tool has the following command line syntax:
```
validate-regulators-config.py -c <config file> -s config_schema.json
```
where `<config file>` is the name of the config file to validate.


## Installation

### Standard Directory

`/usr/share/phosphor-regulators`

The standard version of the config file is installed in this read-only
directory as part of the firmware image install.  This is the config file that
will normally be used.

### Test Directory

`/etc/phosphor-regulators`

A new version of the config file can be tested by copying it into this writable
directory.  This avoids the need to build and install a new firmware image on
the BMC.

### Search Order

The `phosphor-regulators` application will search the installation directories
in the following order to find a config file:
1. test directory
2. standard directory


## Loading and Reloading

The config file is loaded when the `phosphor-regulators` application starts.

To force the application to reload the config file, use the following command
on the BMC:

``systemctl kill -s HUP phosphor-regulators.service``

To confirm which config file was loaded, use the following command on the BMC:

``journalctl -u phosphor-regulators.service | grep Loading``


## Testing

After creating or modifying a config file, you should test it to ensure it
provides the desired behavior.

Perform the following steps to test the config file:
* Run the [validation tool](#validation) to ensure the config file contains no
  errors.
* Copy the config file into the [test directory](#test-directory) on the BMC.
* Force the `phosphor-regulators` application to
  [reload](#loading-and-reloading) its config file, causing it to find and load
  the file in the test directory.
* Boot the system.  Regulator configuration changes (such as voltage settings)
  are only applied during the boot.

When finished testing, perform the following steps to revert to the standard
config file:
* Remove the config file from the test directory.
* Force the `phosphor-regulators` application to reload its config file,
  causing it to find and load the file in the standard directory.
* Boot the system, if necessary, to apply the regulator configuration changes
  in the standard config file.
