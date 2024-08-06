# set_device

## Description

Sets the hardware device that will be used by subsequent actions.

Many actions read from or write to a device. Initially this is the
[device](device.md) that contains the regulator operation being performed, such
as [configuration](configuration.md) or
[sensor_monitoring](sensor_monitoring.md).

Use "set_device" if you need to change the hardware device used by actions. For
example, you need to check a bit in two different I/O expanders to detect a
phase fault.

## Property Value

String containing the unique ID of the [device](device.md).

## Return Value

true

## Example

```json
{
  "set_device": "io_expander2"
}
```
