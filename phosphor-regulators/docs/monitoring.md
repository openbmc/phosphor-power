# Regulator Monitoring

## Overview

The `phosphor-regulators` application supports two types of regulator
monitoring:

- [Sensor monitoring](sensor_monitoring.md)
  - Reading sensor values such as voltage output and temperature
- [Phase fault monitoring](phase_fault_monitoring.md)
  - Checking if a redundant regulator phase has failed

Monitoring is optional and is defined in the
[JSON config file](config_file/README.md)

## How monitoring is enabled

Regulator monitoring is enabled during the system boot after regulators are
enabled (turned on).

The systemd service file
[phosphor-regulators-monitor-enable.service](../../services/phosphor-regulators-monitor-enable.service)
is started. This runs the `regsctl` utility. This utility invokes the D-Bus
`monitor` method on the `phosphor-regulators` application. The parameter value
`true` is passed to the method.

`phosphor-regulators` will start performing the monitoring defined in the JSON
config file. Monitoring is done periodically based on a timer.

## How monitoring is disabled

Regulator monitoring is disabled at the beginning of system shutdown before
regulators are disabled (turned off).

The systemd service file
[phosphor-regulators-monitor-disable.service](../../services/phosphor-regulators-monitor-disable.service)
is started. This runs the `regsctl` utility. This utility invokes the D-Bus
`monitor` method on the `phosphor-regulators` application. The parameter value
`false` is passed to the method.

`phosphor-regulators` will stop performing the monitoring defined in the JSON
config file.
