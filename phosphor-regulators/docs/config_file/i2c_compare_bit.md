# i2c_compare_bit

## Description

Compares a bit in a device register to a value. Communicates with the device
directly using the [I2C interface](i2c_interface.md).

## Properties

| Name     | Required | Type   | Description                                                                                                 |
| :------- | :------: | :----- | :---------------------------------------------------------------------------------------------------------- |
| register |   yes    | string | Device register address expressed in hexadecimal. Must be prefixed with 0x and surrounded by double quotes. |
| position |   yes    | number | Bit position value from 0-7. Bit 0 is the least significant bit.                                            |
| value    |   yes    | number | Expected bit value: 0 or 1.                                                                                 |

## Return Value

Returns true if the register bit contained the expected value, otherwise returns
false.

## Example

```json
{
  "comments": ["Check if bit 3 is on in register 0xA0"],
  "i2c_compare_bit": {
    "register": "0xA0",
    "position": 3,
    "value": 1
  }
}
```
