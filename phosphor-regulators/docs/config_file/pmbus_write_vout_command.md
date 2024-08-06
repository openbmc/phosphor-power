# pmbus_write_vout_command

## Description

Writes the value of VOUT_COMMAND to set the output voltage of a PMBus regulator
rail. Communicates with the device directly using the
[I2C interface](i2c_interface.md).

This action should be executed during [configuration](configuration.md) for the
rail.

### Data Format

The PMBus specification defines four modes/formats for the value of
VOUT_COMMAND:

- Linear
- VID
- Direct
- IEEE Half-Precision Floating Point

Currently only the linear format is supported. The decimal value of the "volts"
property is converted into linear format before being written.

### Exponent For Linear Data Format

The linear data format requires an exponent value.

If the device supports the PMBus VOUT_MODE command, the exponent value can be
read from the device.

If VOUT_MODE is not supported by the device, the exponent value must be
specified using the "exponent" property. The exponent value can normally be
found in the device documentation (data sheet).

### Write Verification

If you wish to verify that the specified volts value was successfully written to
VOUT_COMMAND, specify the "is_verified" property with a value of true.

The value of VOUT_COMMAND will be read from the device after it is written to
ensure that it contains the expected value. If VOUT_COMMAND contains an
unexpected value, an error will be logged and no further configuration will be
performed for this regulator rail.

To perform verification, the device must return all 16 bits of voltage data that
were written to VOUT_COMMAND. The PMBus specification permits a device to have
less than 16 bit internal data resolution, resulting in some low order bits
being zero when read back. However, verification is not supported on devices
that provide less than 16 bit internal data resolution.

## Properties

| Name        | Required | Type                    | Description                                                                                                                                          |
| :---------- | :------: | :---------------------- | :--------------------------------------------------------------------------------------------------------------------------------------------------- |
| volts       |    no    | number                  | Volts value to write, expressed as a decimal number. If not specified, the "volts" property from the [configuration](configuration.md) will be used. |
| format      |   yes    | string                  | Data format of the value written to VOUT_COMMAND. Currently the only supported format is "linear".                                                   |
| exponent    |    no    | number                  | Exponent value for linear data format. Can be positive or negative. If not specified, the exponent value will be read from VOUT_MODE.                |
| is_verified |    no    | boolean (true or false) | If true, the updated value of VOUT_COMMAND is verified by reading it from the device. If false or not specified, the updated value is not verified.  |

## Return Value

true

## Examples

```json
{
  "comments": [
    "Set output voltage.  Get volts value from configuration.",
    "Get exponent from VOUT_MODE."
  ],
  "pmbus_write_vout_command": {
    "format": "linear"
  }
}
```

```json
{
  "comments": [
    "Set output voltage.  Explicitly specify volts and exponent.",
    "Verify value was successfully written to VOUT_COMMAND."
  ],
  "pmbus_write_vout_command": {
    "volts": 1.03,
    "format": "linear",
    "exponent": -8,
    "is_verified": true
  }
}
```
