# not

## Description

Negates the return value of the specified action.

## Property Value

[Action](action.md) to execute.

## Return Value

Returns true if the action returned false. Returns false if the action returned
true.

## Example

```json
{
  "comments": ["Check if register 0xA0 is not equal to 0xFF"],
  "not": {
    "i2c_compare_byte": { "register": "0xA0", "value": "0xFF" }
  }
}
```
