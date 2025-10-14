# template_variable_values

## Description

Template variable values for a specific chassis.

In a multiple chassis system, two or more of the [chassis](chassis.md) may have
the same hardware design.

A [chassis template](chassis_template.md) can be used to avoid duplicate data.
The chassis template defines the power sequencers, GPIOs, and voltage rails in a
chassis. One or more property values in the template contain variables, such as
`${chassis_number}`, to support chassis-specific values.

Each chassis using a template must specify values for each template variable.
For example, the value of `${chassis_number}` is "1" for the first chassis and
"2" for the second chassis.

All chassis variable values are specified as a string (surrounded by double
quotes). If the property value where the variable occurs has a different data
type, such as number, the string will be converted to the required type.

## Properties

| Name                                  | Required | Type   | Description                                             |
| :------------------------------------ | :------: | :----- | :------------------------------------------------------ |
| Variable 1 name (see [notes](#notes)) |   yes    | string | Value of the template variable with the specified name. |
| Variable 2 name (see [notes](#notes)) |   yes    | string | Value of the template variable with the specified name. |
| ...                                   |          |        |                                                         |
| Variable N name (see [notes](#notes)) |   yes    | string | Value of the template variable with the specified name. |

### Notes

- The variable names are defined in the chassis template, such as
  "chassis_number" and "sequencer_bus_number".
- The variable name must be specified without the "$", "{", and "}" characters.

## Example

```json
{
  "chassis_number": "2",
  "sequencer_bus_number": "13"
}
```
