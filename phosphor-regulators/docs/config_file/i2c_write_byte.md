# i2c_write_byte

## Description

Writes a byte to a device register. Communicates with the device directly using
the [I2C interface](i2c_interface.md).

## Properties

| Name     | Required | Type   | Description                                                                                                                                                                                                    |
| :------- | :------: | :----- | :------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| register |   yes    | string | Device register address expressed in hexadecimal. Must be prefixed with 0x and surrounded by double quotes.                                                                                                    |
| value    |   yes    | string | Byte value to write expressed in hexadecimal. Must be prefixed with 0x and surrounded by double quotes.                                                                                                        |
| mask     |    no    | string | Bit mask expressed in hexadecimal. Must be prefixed with 0x and surrounded by double quotes. Specifies which bits to write within the byte value. Only the bits with a value of 1 in the mask will be written. |

## Return Value

true

## Example

```json
{
  "i2c_write_byte": {
    "register": "0x0A",
    "value": "0xCC"
  }
}
```
