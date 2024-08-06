# phosphor-regulators

## Overview

The `phosphor-regulators` application configures and monitors voltage
regulators. The application is controlled by a JSON configuration file.

The application does not control how voltage regulators are enabled or how to
monitor their Power Good (pgood) status. Those operations are typically
performed by power sequencer hardware and related firmware.

## Configuring Voltage Regulators

The configuration of voltage regulators can be modified. Configuration changes
usually override hardware default settings.

The most common configuration change is setting the output voltage for a
regulator rail. Other examples include modifying pgood thresholds and
overcurrent settings.

The configuration changes are applied early in the boot process before
regulators are enabled.

## Monitoring Voltage Regulators

Two types of regulator monitoring are supported:

- Sensor monitoring
- Phase fault monitoring

### Sensor Monitoring

The sensor values for a voltage rail are read, such as voltage output, current
output, and temperature. Sensor values are measured, actual values rather than
target values.

Sensors are read once per second. The sensor values are stored on D-Bus on the
BMC, making them available to external interfaces like Redfish.

### Phase Fault Monitoring

Some voltage regulators contain redundant phases. If a redundant phase fails,
the regulator can continue functioning normally. However redundancy has been
lost, and the regulator may fail if another phase fails.

Voltage regulators can be monitored for redundant phase faults. If a fault is
detected, an error is logged on the BMC.

## JSON Configuration File

The JSON configuration file defines the following:

- Voltage regulators in the system.
- Operations to perform on those regulators, such as configuration or sensor
  monitoring.

Configuration files are stored in the `config_files` directory.

See the [configuration file documentation](docs/config_file/README.md) for
information on the file format, validation tool, and installation directories.
