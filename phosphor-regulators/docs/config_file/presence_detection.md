# presence_detection

## Description

Specifies how to detect whether a device is present.

Some devices are only present in certain system configurations. For example:

- A regulator is only present when a related processor or memory module is
  present.
- A system supports multiple storage backplane types, and the device only exists
  on one of the backplanes.

Device presence is detected by executing actions, such as
[compare_presence](compare_presence.md) and [compare_vpd](compare_vpd.md).

Device operations like [configuration](configuration.md),
[sensor monitoring](sensor_monitoring.md), and
[phase fault detection](phase_fault_detection.md) will only be performed if the
actions indicate the device is present.

The actions can be specified in two ways:

- Use the "rule_id" property to specify a standard rule to run.
- Use the "actions" property to specify an array of actions that are unique to
  this device.

The return value of the rule or the last action in the array indicates whether
the device is present. A return value of true means the device is present; false
means the device is missing.

Device presence will only be detected once per boot of the system. Presence will
be determined prior to the first device operation (such as configuration). When
the system is re-booted, presence will be re-detected. As a result, presence
detection is not supported for devices that can be removed or added
(hot-plugged) while the system is booted and running.

## Properties

| Name     |      Required       | Type                          | Description                                                  |
| :------- | :-----------------: | :---------------------------- | :----------------------------------------------------------- |
| comments |         no          | array of strings              | One or more comment lines describing the presence detection. |
| rule_id  | see [notes](#notes) | string                        | Unique ID of the [rule](rule.md) to execute.                 |
| actions  | see [notes](#notes) | array of [actions](action.md) | One or more actions to execute.                              |

### Notes

- You must specify either "rule_id" or "actions".

## Examples

```json
{
  "comments": ["Regulator is only present on the FooBar backplane"],
  "rule_id": "is_foobar_backplane_installed_rule"
}
```

```json
{
  "comments": ["Regulator is only present when CPU 3 is present"],
  "actions": [
    {
      "compare_presence": {
        "fru": "system/chassis/motherboard/cpu3",
        "value": true
      }
    }
  ]
}
```
