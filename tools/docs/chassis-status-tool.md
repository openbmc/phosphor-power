# Chassis Status Tool

A tool that enables a user to view the status of a system in regard to chassis
status properties including: Presence, Availability, Enabled/Disabled, Power
State, Power Good, Input Power Status, and Power Supply Status. Optionally
includes PLDM state sensor data (Availability, Presence, Power State, and
Operational Fault Status).

## Intention

The intention of this tool is to display the status information of any number of
the chassis on the system onto the command line.

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

-p <PROPERTY>
    - Display only the specified properties. Valid properties:
      Present, Available, Enabled, PowerState, PowerGood,
      InputPowerStatus, PowerSupplyStatus,
      OperationalFaultStatus (PLDM only)
      If not specified, displays all properties.

-v, --verbose
    - Include D-Bus object paths, interface names, and error details for each
      property. For PLDM properties, includes the sensor ID, TID, and FRU
      serial numbers.

--pldm
    - Also display PLDM-sourced chassis status properties. Reads state sensor
      PDRs from pldmd via D-Bus and sensor values directly over MCTP. PLDM
      properties displayed:
        Availability, Present, Power State, Operational Fault Status

-m, --mctp_eid <INT>
    - MCTP endpoint ID. Must be used with --pldm. Defaults to 8.

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
$ chassis-status-tool -c 1 -p Enabled PowerState

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
    Power Supply Status: Unknown
       Power supplies power Status property value could not be obtained.
       Object Path: /xyz/openbmc_project/power/power_supplies/chassis1/psus
       Interface: xyz.openbmc_project.State.Decorator.PowerSystemInputs
```

- Display a single chassis with PLDM properties included, with verbose output
  showing sensor IDs and FRU serial numbers:

```text
$ chassis-status-tool -c 1 --pldm -v

Chassis 1:
    Present: True
       Object Path: /xyz/openbmc_project/inventory/system/chassis1
       Interface: xyz.openbmc_project.Inventory.Item
    ...
PLDM
    Availability: Enabled
       sensorID: 5  TID: 8
    Present: Present
       sensorID: 6  TID: 8
    Power State: On
       sensorID: 7  TID: 8
    Operational Fault Status: Normal
       sensorID: 8  TID: 8
       Serial Number (PLDM):      ABC123
       Serial Number (Inventory): ABC123
```
