# Regulator Configuration

## Overview

The configuration of voltage regulators can be modified. Configuration changes
usually override hardware default settings.

The most common configuration change is setting the output voltage for a
regulator rail. Other examples include modifying pgood thresholds and
overcurrent settings.

Regulator configuration occurs early in the system boot before regulators have
been enabled (turned on).

## How configuration is defined

Configuration is defined for a voltage regulator using the
[configuration](config_file/configuration.md) object in the
[JSON config file](config_file/README.md).

This object specifies actions to run, such as
[pmbus_write_vout_command](config_file/pmbus_write_vout_command.md) and
[i2c_write_byte](config_file/i2c_write_byte.md).

## How configuration is initiated

During the system boot, the systemd service file
[phosphor-regulators-config.service](../../services/phosphor-regulators-config.service)
is started. This runs the `regsctl` utility. This utility invokes the D-Bus
`configure` method on the `phosphor-regulators` application.
`phosphor-regulators` will perform the configuration defined in the JSON config
file.

## Error handling

If an error occurs while executing actions to perform configuration:

- The error will be logged.
- Any remaining actions for the current voltage regulator/rail will be skipped.
- Configuration changes will still be applied to all remaining voltage
  regulators in the system.
- The system boot will continue.
