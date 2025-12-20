# status_monitoring

## Description

In a multiple chassis system, it may be necessary to monitor the
[chassis status](../chassis_status.md) of each chassis. For example, a chassis
might be missing, or it might not have valid input power.

If a chassis has an invalid status, it should not be
[powered on](../powering_on.md) when the overall system is being powered on. It
may also not be possible to
[read the chassis power good signal](../monitoring_chassis_pgood.md).

The "status_monitoring" object is used to specify what types of monitoring to
perform on a chassis.

## Properties

| Name                               | Required | Type                    | Description                                                                                                                                                                          |
| :--------------------------------- | :------: | :---------------------- | :----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| is_present_monitored               |    no    | boolean (true or false) | If true, the Present D-Bus property for this chassis will be monitored to check if the chassis is present. The default value of this JSON property is false.                         |
| is_available_monitored             |    no    | boolean (true or false) | If true, the Available D-Bus property for this chassis will be monitored to check if communication is possible. The default value of this JSON property is false.                    |
| is_enabled_monitored               |    no    | boolean (true or false) | If true, the Enabled D-Bus property for this chassis will be monitored to check if the chassis is in hardware isolation (guarded). The default value of this JSON property is false. |
| is_input_power_status_monitored    |    no    | boolean (true or false) | If true, the Status D-Bus property for this chassis will be monitored to check if the chassis input power is valid. The default value of this JSON property is false.                |
| is_power_supplies_status_monitored |    no    | boolean (true or false) | If true, the Status D-Bus property for this chassis will be monitored to check if the power supply power status is valid. The default value of this JSON property is false.          |

## Example

```json
{
  "is_present_monitored": true,
  "is_enabled_monitored": true,
  "is_power_supplies_status_monitored": true
}
```
