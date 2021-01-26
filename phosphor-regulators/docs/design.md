# Design

This document describes the high-level design of the `phosphor-regulators`
application.

The low-level design is documented using doxygen comments in the source files.

See [README.md](../README.md) for an overview of the functionality provided by
this application.


## Overview

The `phosphor-regulators` application is a single-threaded C++ executable.  It
is a 'daemon' process that runs continually.  The application is launched by
systemd when the BMC reaches the Ready state and before the chassis is powered
on.

The application is driven by a system-specific JSON configuration file.  The
JSON file is found and parsed at runtime.  The parsing process creates a
collection of C++ objects.  These objects implement the regulator configuration
and monitoring behavior that was specified in the JSON file.


## Key Classes

* Manager
  * Top level class created in `main()`.
  * Loads the JSON configuration file.
  * Implements the D-Bus `configure` and `monitor` methods.
  * Contains a System object.
* System
  * Represents the computer system being controlled and monitored by the BMC.
  * Contains one or more Chassis objects.
* Chassis
  * Represents an enclosure that can be independently powered off and on by the
    BMC.
  * Small and mid-sized systems may contain a single Chassis.
  * In a large rack-mounted system, each drawer may correspond to a Chassis.
  * Contains one or more Device objects.
* Device
  * Represents a hardware device, such as a voltage regulator or I/O expander.
  * Contains zero or more Rail objects.
* Rail
  * Represents a voltage rail produced by a voltage regulator, such as 1.1V.


## Regulator Configuration

Regulator configuration occurs early in the system boot before regulators have
been enabled (turned on).

A systemd service file runs the `regsctl` utility.  This utility invokes the
D-Bus `configure` method on the `phosphor-regulators` application.

This D-Bus method is implemented by the Manager object.  The Manager object
calls the C++ `configure()` method on all the objects representing the system
(System, Chassis, Device, and Rail).

The configuration changes are applied to a Device or Rail by executing one or
more actions, such as
[pmbus_write_vout_command](config_file/pmbus_write_vout_command.md).

If an error occurs while executing actions:
* The error will be logged.
* Any remaining actions for the current Device/Rail will be skipped.
* Configuration changes will still be applied to all remaining Device/Rail
  objects in the system.
* The system boot will continue.


## Regulator Monitoring

### Enabling Monitoring

Regulator monitoring is enabled during the system boot after regulators are
enabled (turned on).

A systemd service file runs the `regsctl` utility.  This utility invokes the
D-Bus `monitor` method on the `phosphor-regulators` application.  The parameter
value `true` is passed to the method.

This D-Bus method is implemented by the Manager object.  The Manager object
starts a timer.  The timer periodically calls C++ monitoring methods on all the
objects representing the system (System, Chassis, Device, and Rail).

### Disabling Monitoring

Regulator monitoring is disabled at the beginning of system shutdown before
regulators are disabled (turned off).

A systemd service file runs the `regsctl` utility.  This utility invokes the
D-Bus `monitor` method on the `phosphor-regulators` application.  The parameter
value `false` is passed to the method.

This D-Bus method is implemented by the Manager object.  The Manager object
stops the timer that was periodically calling C++ monitor methods.

### Sensor Monitoring

When regulator monitoring is enabled, sensor values are read once per second.
The timer in the Manager object calls the `monitorSensors()` method on all the
objects representing the system (System, Chassis, Device, and Rail).

The sensor values for a Rail (such as iout, vout, and temperature) are read
using [pmbus_read_sensor](config_file/pmbus_read_sensor.md) actions.

The first time a sensor value is read, a corresponding Sensor object is created
on D-Bus.  The D-Bus object implements the Sensor.Value and OperationalStatus
interfaces.  On subsequent reads, the existing D-Bus Sensor object is updated
with the new sensor value.

An existing D-Bus Sensor object is removed from D-Bus if no corresponding
sensor values are read during monitoring.  This can occur in the following
cases:
* The regulator has been removed from the system (no longer present).
* The regulator was replaced, and the new regulator supports a different set of
  sensors values.  For example, temperature_peak is no longer provided.

If an error occurs while reading the sensors for a Rail:
* The error will be logged.  If the same error occurs repeatedly on a Rail, it
  will only be logged once per system boot.
* Any remaining actions for the Rail will be skipped.
* The value of all D-Bus Sensor objects for this Rail will be set to 0.
* The Functional property of all D-Bus Sensor objects for this Rail will be set
  to false.
* Sensor monitoring will continue.
* The sensors for this Rail will be read again during the next monitoring
  cycle.

If, after an error occurs, a subsequent attempt to read the sensors for a Rail
is successful:
* The D-Bus Sensor objects for this Rail will be set to the new sensor values.
* The Functional property of the D-Bus Sensor objects for this Rail will be set
  to true.

When regulator monitoring is disabled, the Manager object calls the
`disableSensors()` method on all the objects representing the system (System,
Chassis, Device, and Rail).  Each D-Bus Sensor object is set to the special
value NaN to indicate the Sensor is inactive.
