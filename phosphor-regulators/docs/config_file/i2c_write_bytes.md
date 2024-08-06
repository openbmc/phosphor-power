# i2c_write_bytes

## Description

Writes bytes to a device register. Communicates with the device directly using
the [I2C interface](i2c_interface.md).

All of the bytes will be written in a single I2C operation.

The bytes must be specified in the order required by the device. For example, a
PMBus device requires byte values to be written in little-endian order (least
significant byte first).

## Properties

| Name     | Required | Type             | Description                                                                                                                                                                                                                                                                                                                          |
| :------- | :------: | :--------------- | :----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| register |   yes    | string           | Device register address expressed in hexadecimal. Must be prefixed with 0x and surrounded by double quotes. This is the location of the first byte.                                                                                                                                                                                  |
| values   |   yes    | array of strings | One or more byte values to write expressed in hexadecimal. Each value must be prefixed with 0x and surrounded by double quotes.                                                                                                                                                                                                      |
| masks    |    no    | array of strings | One or more bit masks expressed in hexadecimal. Each mask must be prefixed with 0x and surrounded by double quotes. The number of bit masks must match the number of byte values to write. Each mask specifies which bits to write within the corresponding byte value. Only the bits with a value of 1 in the mask will be written. |

## Return Value

true

## Examples

```json
{
  "comments": [
    "Write 0xFF01 to register 0xA0.  Device requires bytes to be",
    "written in big-endian order."
  ],
  "i2c_write_bytes": {
    "register": "0xA0",
    "values": ["0xFF", "0x01"]
  }
}
```

```json
{
  "comments": [
    "Write 0x7302 to register 0x82.  Device requires bytes to be",
    "written in little-endian order.  Do not write the most",
    "significant bit in each byte because the bit is reserved."
  ],
  "i2c_write_bytes": {
    "register": "0x82",
    "values": ["0x02", "0x73"],
    "masks": ["0x7F", "0x7F"]
  }
}
```
