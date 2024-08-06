# i2c_compare_byte

## Description

Compares a device register to a byte value. Communicates with the device
directly using the [I2C interface](i2c_interface.md).

## Properties

| Name     | Required | Type   | Description                                                                                                                                                                                                               |
| :------- | :------: | :----- | :------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------ |
| register |   yes    | string | Device register address expressed in hexadecimal. Must be prefixed with 0x and surrounded by double quotes.                                                                                                               |
| value    |   yes    | string | Expected byte value expressed in hexadecimal. Must be prefixed with 0x and surrounded by double quotes.                                                                                                                   |
| mask     |    no    | string | Bit mask expressed in hexadecimal. Must be prefixed with 0x and surrounded by double quotes. Specifies which bits should be compared within the byte value. Only the bits with a value of 1 in the mask will be compared. |

## Return Value

Returns true if the register contained the expected value, otherwise returns
false.

## Examples

```json
{
  "comments": ["Check if register 0xA0 contains 0xFF"],
  "i2c_compare_byte": {
    "register": "0xA0",
    "value": "0xFF"
  }
}
```

```json
{
  "comments": [
    "Check if register 0x82 contains 0x40.",
    "Ignore the most significant bit."
  ],
  "i2c_compare_byte": {
    "register": "0x82",
    "value": "0x40",
    "mask": "0x7F"
  }
}
```
