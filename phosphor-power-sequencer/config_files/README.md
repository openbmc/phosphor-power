## Overview

The `UCD90xMonitor` class and its subclasses determine the device configuration
from a JSON configuration file. The configuration file describes the power
sequencer rails and power good (pgood) pins defined in the system.

## File Name

The expected filename format is `<device>Monitor_<systemType>.json`, where
`<device>` is the specific power sequencer device type (UCD90320 or UCD90160),
and `<systemType>` is a `Compatible System` name from a `Chassis` Entity Manager
configuration file.

Example file name:

```
UCD90320Monitor_mysystem.json
```

## File Format

The configuration consists of an array of `rails` and an array of `pins`. Both
`rails` and `pins` use a `name` element. The name element should be mnemonic and
unique. If the BMC system utilizes the openpower-pels extension, then it will be
used as an index into the `phosphor-logging` Message Registry.

Example `rails` array with `name` elements:

```
"rails": [
    { "name": "12.0V" },
    { "name": "VDN_DCM0" }
]
```

The `pins` array also uses a `line` element which is the GPIO line number of the
pgood pin in the power sequencer device.

Example `pins` array with `name` and `line` elements:

```
"pins": [
    {
        "name": "5.0V",
        "line": 67
    },
    {
        "name": "VCS_DCM0",
        "line": 74
    }
]
```

Both `rails` and `pins` can optionally define a `presence` element which is an
inventory path to a system component which must be present in order for the rail
or pin to be used in failure isolation. By default, a rail or pin is considered
to be present, which could mean the necessary parts are always present in the
system or that a not present part would not cause a pgood failure.

Example `rails` array element including `presence` element:

```
{
    "name": "VCS1",
    "presence": "/xyz/openbmc_project/inventory/system/chassis/motherboard/vrm1"
}
```

Example `pins` array element including `presence` element:

```
{
    "name": "VPCIE",
    "line": 61,
    "presence": "/xyz/openbmc_project/inventory/system/chassis/motherboard/vrm0"
}
```
