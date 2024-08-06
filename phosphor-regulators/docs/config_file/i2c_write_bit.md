# i2c_write_bit

## Description

Writes a bit to a device register. Communicates with the device directly using
the [I2C interface](i2c_interface.md).

## Properties

| Name     | Required | Type   | Description                                                                                                 |
| :------- | :------: | :----- | :---------------------------------------------------------------------------------------------------------- |
| register |   yes    | string | Device register address expressed in hexadecimal. Must be prefixed with 0x and surrounded by double quotes. |
| position |   yes    | number | Bit position value from 0-7. Bit 0 is the least significant bit.                                            |
| value    |   yes    | number | Value to write: 0 or 1.                                                                                     |

## Return Value

true

## Example

```json
{
  "i2c_write_bit": {
    "register": "0xA0",
    "position": 3,
    "value": 0
  }
}
```
