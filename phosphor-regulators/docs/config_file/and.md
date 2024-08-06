# and

## Description

Tests whether **all** of the actions in an array return true.

Note: All actions in the array will be executed even if an action before the end
returns false. This ensures that actions with beneficial side-effects are always
executed, such as a register read that clears latched fault bits.

## Property Value

Array of two or more [actions](action.md) to execute.

## Return Value

Returns true if **all** of the actions in the array returned true, otherwise
returns false.

## Example

```json
{
  "comments": ["Check whether registers 0xA0 and 0xA1 both contain 0x00"],
  "and": [
    { "i2c_compare_byte": { "register": "0xA0", "value": "0x00" } },
    { "i2c_compare_byte": { "register": "0xA1", "value": "0x00" } }
  ]
}
```
