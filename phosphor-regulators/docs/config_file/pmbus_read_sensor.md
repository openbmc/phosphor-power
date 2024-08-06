# pmbus_read_sensor

## Description

Reads one sensor for a PMBus regulator rail. Communicates with the device
directly using the [I2C interface](i2c_interface.md).

This action should be executed during [sensor_monitoring](sensor_monitoring.md)
for the rail.

### Sensor Type

Currently the following sensor types are supported:

| Type             | Description            |
| :--------------- | :--------------------- |
| iout             | Output current         |
| iout_peak        | Highest output current |
| iout_valley      | Lowest output current  |
| pout             | Output power           |
| temperature      | Temperature            |
| temperature_peak | Highest temperature    |
| vout             | Output voltage         |
| vout_peak        | Highest output voltage |
| vout_valley      | Lowest output voltage  |

Notes:

- Some regulators only support a subset of these sensor types.
- Some of these sensor types are not part of the PMBus specification and must be
  obtained by reading from manufacturer-specific commands.

### Data Format

Currently the following PMBus data formats are supported:

| Format    | Description                                                                                                                                                                                                                |
| :-------- | :------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| linear_11 | Linear data format used for values not related to voltage output, such as output current, input voltage, and temperature. Two byte value with an 11-bit, two's complement mantissa and a 5-bit, two's complement exponent. |
| linear_16 | Linear data format used for values related to voltage output. Two byte (16-bit), unsigned integer that is raised to the power of an exponent. The exponent is not stored within the two bytes.                             |

### Exponent For "linear_16" Data Format

The "linear_16" data format requires an exponent value.

If the device supports the PMBus VOUT_MODE command, the exponent value can be
read from the device.

If VOUT_MODE is not supported by the device, the exponent value must be
specified using the "exponent" property. The exponent value can normally be
found in the device documentation (data sheet).

### D-Bus Sensor

A
[D-Bus sensor object](https://github.com/openbmc/docs/blob/master/architecture/sensor-architecture.md)
will be created on the BMC to store the sensor value. This makes the sensor
available to external interfaces like Redfish.

D-Bus sensors have an object path with the following format:

```text
/xyz/openbmc_project/sensors/<namespace>/<sensor_name>
```

The D-Bus sensors `<namespace>` is the general category of the sensor. The
following table shows how the sensor type is mapped to a D-Bus sensors
`<namespace>`.

| Sensor Type      | D-Bus `<namespace>` |
| :--------------- | :------------------ |
| iout             | current             |
| iout_peak        | current             |
| iout_valley      | current             |
| pout             | power               |
| temperature      | temperature         |
| temperature_peak | temperature         |
| vout             | voltage             |
| vout_peak        | voltage             |
| vout_valley      | voltage             |

The D-Bus `<sensor_name>` must be unique across the entire system. It will be
set to the following:

```text
<rail_id>_<sensor_type>
```

For example, if sensor monitoring for rail "vdd0" reads a "vout_peak" sensor,
the resulting D-Bus `<sensor_name>` will be "vdd0_vout_peak".

Peak and valley sensor values are calculated internally by the regulator since
it can sample values very frequently and catch transient events. When these
peak/valley values are read over PMBus, the regulator will often clear its
internal value and start calculating a new peak/valley. To avoid losing
information, the D-Bus sensor will contain the highest peak/lowest valley value
that has been read since the system was powered on. When the system is powered
off, the D-Bus peak/valley sensor values are cleared.

## Properties

| Name     | Required | Type   | Description                                                                                                                                                   |
| :------- | :------: | :----- | :------------------------------------------------------------------------------------------------------------------------------------------------------------ |
| type     |   yes    | string | Sensor type. Specify one of the following: "iout", "iout_peak", "iout_valley", "pout", "temperature", "temperature_peak", "vout", "vout_peak", "vout_valley". |
| command  |   yes    | string | PMBus command code expressed in hexadecimal. Must be prefixed with 0x and surrounded by double quotes.                                                        |
| format   |   yes    | string | Data format of the sensor value returned by the device. Specify one of the following: "linear_11", "linear_16".                                               |
| exponent |    no    | number | Exponent value for "linear_16" data format. Can be positive or negative. If not specified, the exponent value will be read from VOUT_MODE.                    |

## Return Value

true

## Examples

```json
{
  "comments": ["Read output current from READ_IOUT."],
  "pmbus_read_sensor": {
    "type": "iout",
    "command": "0x8C",
    "format": "linear_11"
  }
}
```

```json
{
  "comments": ["Read output voltage from READ_VOUT.  Specify exponent."],
  "pmbus_read_sensor": {
    "type": "vout",
    "command": "0x8B",
    "format": "linear_16",
    "exponent": -8
  }
}
```
