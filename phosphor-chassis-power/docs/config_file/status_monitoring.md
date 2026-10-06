# status_monitoring

## Description

The "status_monitoring" object specifies what types of monitoring to perform on
the system-level status monitor. The system-level status monitor represents the
entire system (chassis 0).

The only configurable property is `is_available_monitored`. All other status
properties are not configurable.

The system power good property is always monitored.

## Properties

| Name                   | Required | Type                    | Description                                                                                                                                                     |
| :--------------------- | :------: | :---------------------- | :-------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| is_available_monitored |    no    | boolean (true or false) | If true, the Available D-Bus property for the system will be monitored to check if communication is possible. The default value of this JSON property is false. |

## Example

```json
{
  "is_available_monitored": true
}
```
