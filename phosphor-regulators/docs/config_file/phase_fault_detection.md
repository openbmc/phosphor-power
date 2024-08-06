# phase_fault_detection

## Description

Specifies how to detect and log redundant phase faults in a voltage regulator.

A voltage regulator is sometimes called a "phase controller" because it controls
one or more phases that perform the actual voltage regulation.

A regulator may have redundant phases. If a redundant phase fails, the regulator
will continue to provide the desired output voltage. However, a phase fault
error should be logged warning the user that the regulator has lost redundancy.

The technique used to detect a phase fault varies depending on the regulator
hardware. Often a bit is checked in a status register. The status register could
exist in the regulator or in a related I/O expander.

Phase fault detection is performed every 15 seconds. A phase fault must be
detected two consecutive times (15 seconds apart) before an error is logged.
This provides "de-glitching" to ignore transient hardware problems.

Phase faults are detected and logged by executing actions:

- Use the [if](if.md) action to implement the high level behavior "if a fault is
  detected, then log an error".
- Detecting the fault
  - Use a comparison action like [i2c_compare_bit](i2c_compare_bit.md) to detect
    the fault. For example, you may need to check a bit in a status register.
- Logging the error
  - Use the [i2c_capture_bytes](i2c_capture_bytes.md) action to capture
    additional data about the fault if necessary.
  - Use the [log_phase_fault](log_phase_fault.md) action to log a phase fault
    error. The error log will include any data previously captured using
    i2c_capture_bytes.

The actions can be specified in two ways:

- Use the "rule_id" property to specify a standard rule to run.
- Use the "actions" property to specify an array of actions that are unique to
  this regulator.

The default device for the actions is the voltage regulator. You can specify a
different device using the "device_id" property. If you need to access multiple
devices, use the [set_device](set_device.md) action.

## Properties

| Name      |      Required       | Type                          | Description                                                                                                    |
| :-------- | :-----------------: | :---------------------------- | :------------------------------------------------------------------------------------------------------------- |
| comments  |         no          | array of strings              | One or more comment lines describing the phase fault detection.                                                |
| device_id |         no          | string                        | Unique ID of the [device](device.md) to access. If not specified, the default device is the voltage regulator. |
| rule_id   | see [notes](#notes) | string                        | Unique ID of the [rule](rule.md) to execute.                                                                   |
| actions   | see [notes](#notes) | array of [actions](action.md) | One or more actions to execute.                                                                                |

### Notes

- You must specify either "rule_id" or "actions".

## Examples

```json
{
  "comments": ["Detect phase fault using I/O expander"],
  "device_id": "io_expander",
  "rule_id": "detect_phase_fault_rule"
}
```

```json
{
  "comments": [
    "Detect N phase fault using I/O expander.",
    "A fault occurred if bit 3 is ON in register 0x02.",
    "Capture value of registers 0x02 and 0x04 in error log."
  ],
  "device_id": "io_expander",
  "actions": [
    {
      "if": {
        "condition": {
          "i2c_compare_bit": { "register": "0x02", "position": 3, "value": 1 }
        },
        "then": [
          { "i2c_capture_bytes": { "register": "0x02", "count": 1 } },
          { "i2c_capture_bytes": { "register": "0x04", "count": 1 } },
          { "log_phase_fault": { "type": "n" } }
        ]
      }
    }
  ]
}
```
