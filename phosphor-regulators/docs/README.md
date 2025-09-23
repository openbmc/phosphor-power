# phosphor-regulators

## Overview

The `phosphor-regulators` application configures and monitors voltage
regulators. The application is controlled by a JSON configuration file.

The application does not control how voltage regulators are enabled or how to
monitor their Power Good (pgood) status. Those operations are typically
performed by power sequencer hardware and the
[`phosphor-power-sequencer`](../../phosphor-power-sequencer/docs/README.md)
application.

## Application

`phosphor-regulators` is a single-threaded C++ executable. It is a 'daemon'
process that runs continually. The application is launched by systemd when the
BMC reaches the Ready state and before the chassis is powered on.

The application is driven by a system-specific JSON configuration file. The JSON
file is found and parsed at runtime. The parsing process creates a collection of
C++ objects. These objects implement the regulator configuration and monitoring
behavior that was specified in the JSON file.

See [Internal Design](internal_design.md) for more information.

## Configuring voltage regulators

`phosphor-regulators` can modify the configuration of voltage regulators, such
as modifying the output voltage or overcurrent limits.

See [Regulator Configuration](configuration.md) for more information.

## Monitoring voltage regulators

`phosphor-regulators` supports two types of regulator monitoring:

- Sensor monitoring
  - Reading sensor values such as voltage output and temperature
- Phase fault monitoring
  - Checking if a redundant regulator phase has failed

See [Regulator Monitoring](monitoring.md) for more information.

## JSON configuration file

The JSON configuration file defines the following:

- Voltage regulators in the system.
- Operations to perform on those regulators, such as configuration or sensor
  monitoring.

Configuration files are stored in the [`config_files`](../config_files)
directory.

See the [configuration file documentation](docs/config_file/README.md) for
information on the file format, validation tool, and installation directories.

## Testing

Automated test cases exist for most of the code in this application. See
[Testing](testing.md) for more information.
