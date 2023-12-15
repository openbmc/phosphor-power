# gpio

## Description

A General Purpose Input/Output (GPIO) that can be read to obtain the pgood
status of a voltage rail.

GPIO values are read using the libgpiod interface.

## Properties

| Name       | Required | Type                    | Description |
| :--------- | :------: | :---------------------- | :---------- |
| line       |   yes    | number                  | The libgpiod line offset of the GPIO. |
| active_low |    no    | boolean (true or false) | If true, the GPIO value 0 indicates a true pgood status.  If false, the GPIO value 1 indicates a true pgood status.  The default value of this property is false. |

## Example

```
{
    "line": 60
}
```
