## Overview

This directory contains applications for power control and configuring /
monitoring power sequencer and related devices that support JSON-driven
configuration.

The currently implemented application is named `phosphor-power-control` and
supports GPIO based power control and power sequencer monitoring with specific
support for the UCD90320 device available.

## JSON Configuration Files

Configuration files are stored in the `config_files` directory.
See the [Configuration File README](config_files/README.md) for details on the
format of power sequencer configuration files.

## Source Files

Source files are stored in the `src` directory.
See the [Source Files README](src/README.md) for details on the design of the
application source.
