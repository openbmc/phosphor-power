# Phase Fault Monitoring

## Overview

Some voltage regulators contain redundant phases. If a redundant phase fails,
the regulator can continue functioning normally. However redundancy has been
lost, and the regulator may fail if another phase fails.

Voltage regulators can be monitored for redundant phase faults. If a fault is
detected, an error is logged on the BMC.

When [regulator monitoring](monitoring.md) is enabled, the `phosphor-regulators`
application checks for redundant phase faults every 15 seconds.

A phase fault must be detected two consecutive times (15 seconds apart) before
an error is logged. This provides "de-glitching" to ignore transient hardware
problems.

A phase fault error will only be logged for a regulator once per system boot.

## How phase fault detection is defined

Phase fault detection is defined for a voltage regulator using the
[phase_fault_detection](config_file/phase_fault_detection.md) object in the
[JSON config file](config_file/README.md).

This object specifies actions to run, such as
[i2c_compare_byte](config_file/i2c_compare_byte.md).

## Error handling

If a different type of error occurs while detecting phase faults in a regulator:

- The error will be logged. If the same error occurs repeatedly on regulator, it
  will only be logged once per system boot.
- Any remaining actions for the regulator will be skipped.
- Phase fault detection will continue with the next regulator.
- Phase fault detection will be attempted again for this regulator during the
  next monitoring cycle.
