# Sensor Monitoring

## Overview

Sensor values such as output voltage, output current, and temperature can be
read from some voltage regulators. Sensor values are measured, actual values
rather than target values.

When [regulator monitoring](monitoring.md) is enabled, the `phosphor-regulators`
application reads sensor values once per second.

The sensor values are stored on D-Bus on the BMC, making them available to
external interfaces like Redfish.

## How sensor monitoring is defined

Sensor monitoring is defined for a voltage regulator using the
[sensor_monitoring](config_file/sensor_monitoring.md) object in the
[JSON config file](config_file/README.md).

This object specifies actions to run, such as
[pmbus_read_sensor](config_file/pmbus_read_sensor.md).

## How sensors are stored on D-Bus

The first time a sensor value is read, a corresponding sensor object is created
on D-Bus. On subsequent reads, the existing D-Bus sensor object is updated with
the new sensor value.

The D-Bus sensor object implements the following interfaces:

- xyz.openbmc_project.Sensor.Value
- xyz.openbmc_project.State.Decorator.OperationalStatus
- xyz.openbmc_project.State.Decorator.Availability
- xyz.openbmc_project.Association.Definitions

An existing D-Bus Sensor object is removed from D-Bus if no corresponding sensor
values are read during monitoring. This can occur in the following cases:

- The regulator has been removed from the system (no longer present).
- The regulator was replaced, and the new regulator supports a different set of
  sensors values. For example, temperature_peak is no longer provided.
- The chassis containing the regulator has an incorrect status, such as missing
  or no input power.

When regulator monitoring is disabled, the following changes will be made to all
of the D-Bus sensor objects:

- The Value property will be set to NaN.
- The Available property will be set to false.

## Error handling

If an error occurs while reading the sensors for a voltage regulator rail:

- The error will be logged. If the same error occurs repeatedly on a rail, it
  will only be logged once per system boot.
- Any remaining actions for the rail will be skipped.
- The following changes will be made to all D-Bus sensor objects for this rail:
  - The Value property will be set to NaN.
  - The Functional property will be set to false.
- Sensor monitoring will continue with the next rail or voltage regulator.
- The sensors for this rail will be read again during the next monitoring cycle.

If a subsequent attempt to read the sensors for the rail is successful, the
following changes will be made to the D-Bus sensor objects:

- The Value property will be set to the new sensor reading.
- The Functional property will be set to true.
