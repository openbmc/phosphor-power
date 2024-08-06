# i2c_capture_bytes

## Description

Captures device register bytes to be stored in an error log.

Reads the specified device register and temporarily stores the value. If a
subsequent action (such as [log_phase_fault](log_phase_fault.md)) creates an
error log, the captured bytes will be stored in the error log.

This action allows you to capture additional data about a hardware error. The
action can be used multiple times if you wish to capture data from multiple
registers or devices before logging the error.

Communicates with the device directly using the
[I2C interface](i2c_interface.md). All of the bytes will be read in a single I2C
operation.

The bytes will be stored in the error log in the same order as they are received
from the device. For example, a PMBus device transmits byte values in
little-endian order (least significant byte first).

Note: This action should only be used after a hardware error has been detected
to avoid unnecessary I2C operations and memory usage.

## Properties

| Name     | Required | Type   | Description                                                                                                                                         |
| :------- | :------: | :----- | :-------------------------------------------------------------------------------------------------------------------------------------------------- |
| register |   yes    | string | Device register address expressed in hexadecimal. Must be prefixed with 0x and surrounded by double quotes. This is the location of the first byte. |
| count    |   yes    | number | Number of bytes to read from the device register.                                                                                                   |

## Return Value

true

## Example

```json
{
  "comments": ["Capture 2 bytes from register 0xA0 to store in error log"],
  "i2c_capture_bytes": {
    "register": "0xA0",
    "count": 2
  }
}
```
