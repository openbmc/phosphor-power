## Overview

The `UCD90320Monitor' class determines its device configuration from a JSON
configuration file. The configuration file describes the power sequencer rails
and power good (pgood) pins defined in the system.

## File Name

The expected filename format is `UCD90320Monitor_<systemType>.json`, where
`<systemType>` is a `Compatible System` name from a `Chassis` Entity Manager
configuration file.

## File Format

The configuration consists of an array of `rails` and an array of `pins`. Both
`rails` and `pins` use a `name` element. The name element should be pneumonic
and unique. It will be used as an index into the `phosphor-logging` Message
Registry. The `pins` elements also define a `line` element which is the GPIO
line number of the pgood pin. Both `rails` and `pins` can optionally define a
`presence` element which is an inventory path to a system component which must
be presence in order for the rail or pin to be used in failure isolation. By
default, a rail or pin is considered to be present, which could mean the
necessary parts are always present in the system or that a not present part
would not cause a pgood failure.
