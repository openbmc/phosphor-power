# phosphor-power-sequencer Configuration File

## Table of Contents

- [Overview](#overview)
- [Data Format](#data-format)
- [Name](#name)
- [Contents](#contents)
- [Installation](#installation)

## Overview

The `phosphor-power-sequencer` configuration file (config file) contains
information about the power sequencer device within a system. This device is
responsible for enabling the voltage rails in order and monitoring them for
pgood faults.

The information in the config file is used to determine which voltage rail
caused a pgood fault.

The config file is optional. If no file is found, the application will log a
general error when a pgood fault occurs. No specific rail will be identified.

## Data Format

The config file is a text file in the
[JSON (JavaScript Object Notation)](https://www.json.org/) data format.

## Name

The config file name is based on the system type that it supports.

A config file is normally system-specific. Each system type usually has a
different set of voltage rails and GPIOs.

The system type is obtained from a D-Bus Chassis object created by the
[Entity Manager](https://github.com/openbmc/entity-manager) application. The
object must implement the `xyz.openbmc_project.Inventory.Decorator.Compatible`
interface.

The `Names` property of this interface contains a list of one or more compatible
system types. The types are ordered from most specific to least specific.

Example:

- `com.acme.Hardware.Chassis.Model.MegaServer4CPU`
- `com.acme.Hardware.Chassis.Model.MegaServer`
- `com.acme.Hardware.Chassis.Model.Server`

The `phosphor-power-sequencer` application searches for a config file name that
matches one of these compatible system types. It searches from most specific to
least specific. The first config file found, if any, will be used.

For each compatible system type, the application will look for two config file
names:

- The complete compatible system type plus a '.json' suffix
- The last node of the compatible system type plus a '.json' suffix

Example:

- `com.acme.Hardware.Chassis.Model.MegaServer4CPU.json`
- `MegaServer4CPU.json`

## Contents

The config file contains a single JSON [config_file](config_file.md) object at
the root level. That object contains arrays of other JSON objects.

## Installation

The config file is installed in the `/usr/share/phosphor-power-sequencer`
directory on the BMC.
