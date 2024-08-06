# i2c_compare_bytes

## Description

Compares device register bytes to an array of expected values. Communicates with
the device directly using the [I2C interface](i2c_interface.md).

All of the bytes will be read in a single I2C operation.

The bytes must be specified in the same order as they will be received from the
device. For example, a PMBus device transmits byte values in little-endian order
(least significant byte first).

## Properties

| Name     | Required | Type             | Description                                                                                                                                                                                                                                                                                                                                     |
| :------- | :------: | :--------------- | :---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| register |   yes    | string           | Device register address expressed in hexadecimal. Must be prefixed with 0x and surrounded by double quotes. This is the location of the first byte.                                                                                                                                                                                             |
| values   |   yes    | array of strings | One or more expected byte values expressed in hexadecimal. Each value must be prefixed with 0x and surrounded by double quotes.                                                                                                                                                                                                                 |
| masks    |    no    | array of strings | One or more bit masks expressed in hexadecimal. Each mask must be prefixed with 0x and surrounded by double quotes. The number of bit masks must match the number of expected byte values. Each mask specifies which bits should be compared within the corresponding byte value. Only the bits with a value of 1 in the mask will be compared. |

## Return Value

Returns true if all the register bytes contained the expected values, otherwise
returns false.

## Examples

```json
{
  "comments": [
    "Check if register 0xA0 contains 0xFF01.",
    "Device returns bytes in big-endian order."
  ],
  "i2c_compare_bytes": {
    "register": "0xA0",
    "values": ["0xFF", "0x01"]
  }
}
```

```json
{
  "comments": [
    "Check if register 0x82 contains 0x7302.",
    "Device returns bytes in little-endian order.",
    "Ignore the most significant bit in each byte."
  ],
  "i2c_compare_bytes": {
    "register": "0x82",
    "values": ["0x02", "0x73"],
    "masks": ["0x7F", "0x7F"]
  }
}
```
