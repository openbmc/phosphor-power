# log_phase_fault

## Description

Logs a redundant phase fault error for a voltage regulator. This action should
be executed if a fault is detected during
[phase_fault_detection](phase_fault_detection.md) for the regulator.

A regulator may contain one or more redundant phases:

- An "N+2" regulator has two redundant phases
- An "N+1" regulator has one redundant phase

A phase fault occurs when a phase stops functioning properly. The redundancy
level of the regulator is reduced.

The phase fault type indicates the level of redundancy remaining **after** the
fault has occurred:

| Type | Description                                                                                      |
| :--- | :----------------------------------------------------------------------------------------------- |
| n+1  | An "N+2" regulator has lost one redundant phase. The regulator is now at redundancy level "N+1". |
| n    | Regulator has lost all redundant phases. The regulator is now at redundancy level N.             |

If additional data about the fault was previously captured using
[i2c_capture_bytes](i2c_capture_bytes.md), that data will be stored in the error
log.

## Properties

| Name | Required | Type   | Description                                                 |
| :--- | :------: | :----- | :---------------------------------------------------------- |
| type |   yes    | string | Phase fault type. Specify one of the following: "n+1", "n". |

## Return Value

true

## Example

```json
{
  "log_phase_fault": {
    "type": "n+1"
  }
}
```
