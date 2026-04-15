# status_monitoring

## Description

In a multiple chassis system, it may be necessary to monitor the status of each
chassis. For example, a chassis might be missing, or it might be in hardware
isolation (guarded).

If a chassis has an invalid status, regulators within that chassis should not be
configured or monitored.

The "status_monitoring" object is used to specify what types of monitoring to
perform on a chassis.

## Properties

| Name                   | Required | Type                    | Description                                                                                                                                                                          |
| :--------------------- | :------: | :---------------------- | :----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| is_present_monitored   |    no    | boolean (true or false) | If true, the Present D-Bus property for this chassis will be monitored to check if the chassis is present. The default value of this JSON property is false.                         |
| is_available_monitored |    no    | boolean (true or false) | If true, the Available D-Bus property for this chassis will be monitored to check if communication is possible. The default value of this JSON property is false.                    |
| is_enabled_monitored   |    no    | boolean (true or false) | If true, the Enabled D-Bus property for this chassis will be monitored to check if the chassis is in hardware isolation (guarded). The default value of this JSON property is false. |

## Example

```json
{
  "is_present_monitored": true,
  "is_enabled_monitored": true
}
```
