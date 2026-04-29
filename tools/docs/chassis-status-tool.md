# Chassis Status Tool

A tool that enables a user to view the status of a system in regard to chassis
status properties including: Presence, Availability, Enabled/Disabled, Power
State, Power Good, Input Power Status, and Power Supply Status.

## Intention

The intention of this tool is to display the status information of any number of
the chassis on the system onto the command line.

## Number of Chassis

The tool automatically discovers the number of chassis using D-Bus
introspection:

1. **D-Bus Introspection**: Queries `xyz.openbmc_project.Power.Chassis` service
   at `/xyz/openbmc_project/power/chassis` using the
   `org.freedesktop.DBus.Introspectable.Introspect()` method.

2. **Chassis Discovery**: These D-Bus objects are created for all chassis
   defined in the system configuration file (e.g., `Huygens.json`), regardless
   of whether the chassis is physically present

## Usage

```text
NAME
  chassis-status-tool - Display chassis status properties.

SYNOPSIS
  chassis-status-tool [OPTION]
  chassis-status-tool display [OPTION]

OPTIONS
-c <INT>
    - Display the chassis status properties for a given chassis number. Cannot be used with -n option.

-n <INT>
    - Specify the number of chassis to display. Displays all chassis from 0 up to and including the specified number. Cannot be used with -c option.

-p <PROPERTY>...
    - Display only the specified properties. Valid properties:
      present, available, enabled, powerState, powerGood,
      inputPower (Input Power Status), powerSupplies (Power Supply Status)
      If not specified, displays all properties.

-v, --verbose
    - Include D-Bus interface and object paths for each property.

display
    - Optional subcommand. When omitted, the tool defaults to display behavior.
      All options work with or without this subcommand.
```

## Examples

- Display the status properties for every chassis (0-8), even if a chassis is
  not physically present:

```text
$ chassis-status-tool

Chassis 0:
    Present: True
    Available: True
    Enabled: True
    Power State: Power On
    Power Good: Powered On
    Input Power Status: Good
    Power Supply Status: Good

...

Chassis 8:
    Present: True
    Available: True
    Enabled: True
    Power State: Power On
    Power Good: Powered On
    Input Power Status: Good
    Power Supply Status: Good
```

- Display a single chassis:

```text
$ chassis-status-tool display -c 1

Chassis 1:
    Present: True
    Available: True
    Enabled: True
    Power State: Power On
    Power Good: Powered On
    Input Power Status: Good
    Power Supply Status: Good
```

- Display only specific properties for one chassis:

```text
$ chassis-status-tool -c 1 -p enabled powerState

Chassis 1:
    Enabled: True
    Power State: Power Off

```

- Display chassis 0 through 3 only:

```text
$ chassis-status-tool -n 3

Chassis 0:
    Present: True
    Available: True
    Enabled: True
    Power State: Power On
    Power Good: Powered On
    Input Power Status: Good
    Power Supply Status: Good

...

Chassis 3:
    Present: True
    Available: True
    Enabled: True
    Power State: Power On
    Power Good: Powered On
    Input Power Status: Good
    Power Supply Status: Good
```

- Display a single chassis with verbose output:

```text
$ chassis-status-tool display -c 1 -v

Chassis 1:
    Present: True
       Object Path: /xyz/openbmc_project/inventory/system/chassis1
       Interface: xyz.openbmc_project.Inventory.Item
    Available: True
       Object Path: /xyz/openbmc_project/inventory/system/chassis1
       Interface: xyz.openbmc_project.State.Decorator.Availability
    Enabled: True
       Object Path: /xyz/openbmc_project/inventory/system/chassis1
       Interface: xyz.openbmc_project.Object.Enable
    Power State: Power On
       Object Path: /org/openbmc/control/power1
       Interface: org.openbmc.control.Power
    Power Good: Powered On
       Object Path: /org/openbmc/control/power1
       Interface: org.openbmc.control.Power
    Input Power Status: Good
       Object Path: /xyz/openbmc_project/power/chassis/chassis1
       Interface: xyz.openbmc_project.State.Decorator.PowerSystemInputs
    Power Supply Status: Good
       Object Path: /xyz/openbmc_project/power/power_supplies/chassis1/psus
       Interface: xyz.openbmc_project.State.Decorator.PowerSystemInputs
```
