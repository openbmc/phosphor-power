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
```

## Examples

- Dump all chassis information (the tool polls every chassis slot, 0 through 8,
  regardless of whether a chassis is physically present):

  ```text
  $ chassis-status-tool dump

  Chassis 0:
      Present: True
      Available: Unknown
          Available property value could not be obtained.
      Enabled: True
      Power state: Power Off
      Power good: Powered Off
      Input power Status: Unknown
          Input Power Status property value could not be obtained.
      Power supply Status: Good

  ...

  Chassis 8:
      Present: Unknown
          Present property value could not be obtained.
      Available: Unknown
          Available property value could not be obtained.
      Enabled: Unknown
          Enabled property value could not be obtained.
      Power state: Unknown
          Power state property value could not be obtained.
      Power good: Unknown
          Power good property value could not be obtained.
      Input Power Status: Unknown
          Input power Status property value could not be obtained.
      Power Supply Status: Unknown
          Power Supply Status property value could not be obtained.
  ```

- Dump a single chassis:

  ```text
  $ chassis-status-tool dump -c 1

  Chassis 1:
      Present: True
      Available: Unknown
          Available property value could not be obtained.
      Enabled: True
      Power state: Unknown
          Power state property value could not be obtained.
      Power Good: Unknown
          Power good property value could not be obtained.
      Input Power Status: Unknown
          Input power Status property value could not be obtained.
      Power Supply Status: Unknown
          Power supply Status property value could not be obtained.
  ```
