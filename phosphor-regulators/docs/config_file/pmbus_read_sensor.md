# pmbus_read_sensor

## Description
Reads one sensor for a PMBus regulator rail.  Communicates with the device
directly using the [I2C interface](i2c_interface.md).

This action should be executed during [sensor_monitoring](sensor_monitoring.md)
for the rail.

### Sensor Value Type
Currently the following sensor value types are supported:
| Type | Description |
| :--- | :---------- |
| iout | Output current |
| iout_peak | Highest output current |
| iout_valley | Lowest output current |
| pout | Output power |
| temperature | Temperature |
| temperature_peak | Highest temperature |
| vout | Output voltage |
| vout_peak | Highest output voltage |
| vout_valley | Lowest output voltage |

Notes:
* Some regulators only support a subset of these sensor value types.
* Some of these sensor value types are not part of the PMBus specification and
  must be obtained by reading from manufacturer-specific commands.

### Data Format
Currently the following PMBus data formats are supported:
| Format | Description |
| :----- | :---------- |
| linear_11 | Linear data format used for values not related to voltage output, such as output current, input voltage, and temperature.  Two byte value with an 11-bit, two's complement mantissa and a 5-bit, two's complement exponent. |
| linear_16 | Linear data format used for values related to voltage output.  Two byte (16-bit), unsigned integer that is raised to the power of an exponent.  The exponent is not stored within the two bytes. |

### Exponent For "linear_16" Data Format
The "linear_16" data format requires an exponent value.

If the device supports the PMBus VOUT_MODE command, the exponent value can be
read from the device.

If VOUT_MODE is not supported by the device, the exponent value must be
specified using the "exponent" property.  The exponent value can normally be
found in the device documentation (data sheet).

### D-Bus Sensor
A [D-Bus sensor
object](https://github.com/openbmc/docs/blob/master/sensor-architecture.md)
will be created on the BMC to store the sensor value.  This makes the sensor
available to external interfaces like Redfish. 

D-Bus sensors have an object path with the following format:
```
/xyz/openbmc_project/sensors/<type>/<label>
```

The D-Bus `<type>` is the hwmon class name.  The following table shows how the
sensor value type is mapped to a D-Bus `<type>`.
| Sensor Value Type | D-Bus `<type>` |
| :---------------- | :------------- |
| iout | current |
| iout_peak | current |
| iout_valley | current |
| pout | power |
| temperature | temperature |
| temperature_peak | temperature |
| vout | voltage |
| vout_peak | voltage |
| vout_valley | voltage |

The D-Bus `<label>` is the sensor name.  It must be unique within the entire
system.  The `<label>` will be set to the following:
```
<rail_id>_<sensor_value_type>
```
For example, if sensor monitoring for rail "vdd0" reads a "vout_peak" sensor,
the resulting D-Bus `<label>` will be "vdd0_vout_peak".

## Properties
| Name | Required | Type | Description |
| :--- | :------: | :--- | :---------- |
| type | yes | string | Sensor value type.  Specify one of the following: "iout", "iout_peak", "iout_valley", "pout", "temperature", "temperature_peak", "vout", "vout_peak", "vout_valley". |
| command | yes | string | PMBus command code expressed in hexadecimal.  Must be prefixed with 0x and surrounded by double quotes. |
| format | yes | string | Data format of the sensor value returned by the device.  Specify one of the following: "linear_11", "linear_16".
| exponent | no | number | Exponent value for "linear_16" data format.  Can be positive or negative.  If not specified, the exponent value will be read from VOUT_MODE. |

## Return Value
true

## Examples
```
{
  "comments": [ "Read output current from READ_IOUT." ],
  "pmbus_read_sensor": {
    "type": "iout",
    "command": "0x8C",
    "format": "linear_11"
  }
}

{
  "comments": [ "Read output voltage from READ_VOUT.  Specify exponent." ],
  "pmbus_read_sensor": {
    "type": "vout",
    "command": "0x8B",
    "format": "linear_16",
    "exponent": -8
  }
}
```
