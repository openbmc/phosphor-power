## Overview

This directory contains the source code for applications performing system power
on and off control, and power sequencer device configuration and monitoring.

## Class Descriptions

### PowerControl

Implements GPIO control of power on / off and monitoring of the chassis power
good (pgood). The GPIOs are defined in the device tree and are named
`power-chassis-control` and `power-chassis-good` respectively. The chassis pgood
is monitored on a three second poll. Enforces a minimum power off time of 15
seconds from cold start and 25 seconds from power off.

Provides the server implementation of the `org.openbmc.control.Power` D-Bus
interface.

Determines the type and I2C information of the power sequencer device with the
Entity Manager `xyz.openbmc_project.Configuration.UCD90320` or
`xyz.openbmc_project.Configuration.UCD90160` interface. If a specific device
interface is not provided or not desired, a generic base class implementation
(see the `PowerSequencerMonitor` class below) will be used.

### PowerInterface

Defines the `org.openbmc.control.Power` D-Bus interface.

The `state` property is set to initiate a power on or power off sequence. The
power good `pgood` property reflects the power state of the chassis. At power on
time the `pgood` will lag the `state` as the power sequencer performs its
processing. The same lag will occur on a requested power off. Loss of `pgood`
without a `state` change request indicates a pgood failure.

### PowerSequencerMonitor

Implements a generic power sequencer device monitoring interface. Used when the
specific device type is not specified or cannot be determined. Also used as a
base class for specific device classes. The generic implementation does not do
any device specific analysis and does not need Entity Manager or JSON file
configuration.

### UCD90xMonitor

Defines a base class for monitoring the UCD90\* family of power sequencer
devices and implements the common behavior. Uses the Entity Manager
`IBMCompatibleSystem` interface to locate its JSON based configuration. The
loaded configuration allows pgood failure analysis to determine the specific
voltage rail or pgood pin which has failed.

### UCD90320Monitor

Implements a specific UCD90320 power sequencer device monitoring class.

### UCD90160Monitor

Implements a specific UCD90160 power sequencer device monitoring class.
