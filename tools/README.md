# tools

## Overview

This directory contains command-line tools for monitoring and managing power
systems.

## Tools

- [psutils]: A utility for inspecting PSU firmware versions and models,
  comparing firmware versions, and flashing new PSU firmware over I2C.
- [chassis-status-tool](docs/chassis-status-tool.md): A utility for displaying
  chassis status properties.

## Libraries

- [i2c]: A library providing a C++ abstraction over Linux I2C/SMBus
  communication. Used by multiple phosphor-power applications to communicate
  directly with hardware devices.

## Location

Both tools are installed to `/usr/bin`:

- `/usr/bin/psutils`
- `/usr/bin/chassis-status-tool`
