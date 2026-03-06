# Chassis Status Tool

A tool that enables a user to view the status of a system in regard to chassis
status properties including: Presence, Availability, Enabled/Disabled, Power
State, Power Good, Input Power Status, and Power Supply Status.

## Intention

The intention of this tool is to dump the status information of one or all of
the chassis on the system onto the command line.

## Usage

```text
NAME
  chassis-status-tool - Dump chassis status properties.

SYNOPSIS
  chassis-status-tool [OPTION]

OPTIONS
dump
    - Starting at chassis 0 (global system designation for all chassis), dump
      status properties for every possible chassis (0-8).
dump -c <INT>
    - Dump the chassis status properties for a given chassis number.
dump -v
    - Dump the interface and object paths each of the chassis status properties.
```

## Examples

- Dump the status properties for every chassis (0-8), even if a chassis is not
  physically present:

```text
$ chassis-status-tool dump

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

- Dump a single chassis:

  ```text
  $ chassis-status-tool dump -c 1

  Chassis 1:
      Present: True
      Available: True
      Enabled: True
      Power State: Power On
      Power Good: Powered On
      Input Power Status: Good
      Power Supply Status: Good
  ```

- Dump a single chassis with verbose output:

  ```text
  $ chassis-status-tool dump -c 1 -v

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
