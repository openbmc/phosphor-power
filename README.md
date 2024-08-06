# phosphor-power

## Overview

This repository contains applications for configuring and monitoring devices
that deliver power to the system.

Actively-maintained applications:

- cold-redundancy: Application that makes power supplies work in Cold Redundancy
  mode and rotates them at intervals.
- [phosphor-power-sequencer](phosphor-power-sequencer/README.md): JSON-driven
  application that powers the chassis on/off and monitors the power sequencer
  device.
- [phosphor-power-supply](phosphor-power-supply/README.md): Next generation
  power supply monitoring application.
- [phosphor-regulators](phosphor-regulators/README.md): JSON-driven application
  that configures and monitors voltage regulators.
- tools/power-utils: Power supply utilities.

Legacy applications:

- power-sequencer: Original power sequencer monitoring application.
- power-supply: Original power supply monitoring application.

## Build

To build all applications in this repository:

```sh
  meson setup build
  ninja -C build
```

To clean the repository and remove all build output:

```sh
  rm -rf build
```

You can specify [meson options](meson.options) to customize the build process.
For example, you can specify:

- Which applications to build and install.
- Application-specific configuration data, such as power sequencer type.
- Whether to build tests.

## Power Supply Monitor and Util JSON config

Several applications in this repository require a PSU JSON config to run. The
JSON config file provides information for:

- Where to access the pmbus attributes
- Which attribute file in pmbus maps to which property and interface in D-Bus
- Which kernel device directory is used on which PSU

There is an example [psu.json](example/psu.json) to describe the necessary
configurations.

- `inventoryPMBusAccessType` defines the pmbus access type, which tells the
  service which sysfs type to use to read the attributes. The possible values
  are:
  - Base: The base dir, e.g. `/sys/bus/i2c/devices/3-0069/`
  - Hwmon: The hwmon dir, e.g. `/sys/bus/i2c/devices/3-0069/hwmon/hwmonX/`
  - Debug: The pmbus debug dir, e.g. `/sys/kernel/debug/pmbus/hwmonX/`
  - DeviceDebug: The device debug dir, e.g.
    `/sys/kernel/debug/<driver>.<instance>/`
  - HwmonDeviceDebug: The hwmon device debug dir, e.g.
    `/sys/kernel/debug/pmbus/hwmonX/cffps1/`
- `fruConfigs` defines the mapping between the attribute file and the FRU
  inventory interface and property. The configuration example below indicates
  that the service will read `part_number` attribute file from a directory
  specified by the above pmbus access type, and assign to `PartNumber` property
  in `xyz.openbmc_project.Inventory.Decorator.Asset` interface.

  ```json
    "fruConfigs": [
      {
        "propertyName": "PartNumber",
        "fileName": "part_number",
        "interface": "xyz.openbmc_project.Inventory.Decorator.Asset"
      }
    ]
  ```

- `psuDevices` defines the kernel device dir for each PSU in inventory. The
  configuration example below indicates that `powersupply0`'s device is located
  in `/sys/bus/i2c/devices/3-0069`.

  ```json
    "psuDevices": {
      "/xyz/openbmc_project/inventory/system/chassis/motherboard/powersupply0" : "/sys/bus/i2c/devices/3-0069",
    }
  ```
