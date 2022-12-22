## Overview

This directory is a repository for applications which perform power control or
configure or monitor power sequencer or related devices. These applications may
support JSON-driven configuration.

The currently implemented application is named `phosphor-power-control` and
supports GPIO based power control and power sequencer monitoring with specific
support for the UCD90320 and UCD90160 devices available.

## JSON Configuration Files

Configuration files are stored in the `config_files` directory. See the
[Configuration File README](config_files/README.md) for details on the format of
power sequencer configuration files.

## Source Files

Source files are stored in the `src` directory. See the
[Source Files README](src/README.md) for details on the design of the
application source.
