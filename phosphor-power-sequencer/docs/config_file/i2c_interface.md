# i2c_interface

## Description

I2C interface to a device.

## Properties

| Name    | Required | Type   | Description                                                                                                         |
| :------ | :------: | :----- | :------------------------------------------------------------------------------------------------------------------ |
| bus     |   yes    | number | I2C bus number of the device. The first bus is 0.                                                                   |
| address |   yes    | string | 7-bit I2C address of the device expressed in hexadecimal. Must be prefixed with 0x and surrounded by double quotes. |

## Example

```json
{
  "bus": 1,
  "address": "0x70"
}
```
