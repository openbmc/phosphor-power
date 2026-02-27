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
    - Dump the status properties for every chassis (0-8), even if a chassis is
    not physically present.
dump -c <INT>
    - Dump the chassis status properties for a given chassis number.
```

## Examples

- Dump all chassis information (the tool polls every chassis slot, 0 through 8,
  regardless of whether a chassis is physically present):

  ```text
  $ chassis-status-tool dump

  Chassis 0:
      Present: True
      Available: True
      Enabled: True
      Power state: Power On
      Power good: Powered On
      Input power Status: Good
      Power supply Status: Good

  ...

  Chassis 8:
      Present: True
      Available: True
      Enabled: True
      Power state: Power On
      Power good: Powered On
      Input power Status: Good
      Power supply Status: Good
  ```

- Dump a single chassis:

  ```text
  $ chassis-status-tool dump -c 1

  Chassis 1:
      Present: True
      Available: True
      Enabled: True
      Power state: Power On
      Power good: Powered On
      Input power Status: Good
      Power supply Status: Good
  ```
