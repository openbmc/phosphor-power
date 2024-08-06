# or

## Description

Tests whether **any** of the actions in an array return true.

Note: All actions in the array will be executed even if an action before the end
returns true. This ensures that actions with beneficial side-effects are always
executed, such as a register read that clears latched fault bits.

## Property Value

Array of two or more [actions](action.md) to execute.

## Return Value

Returns true if **any** of the actions in the array returned true, otherwise
returns false.

## Example

```json
{
  "comments": ["Check whether register 0xA0 or 0xA1 contains 0x00"],
  "or": [
    { "i2c_compare_byte": { "register": "0xA0", "value": "0x00" } },
    { "i2c_compare_byte": { "register": "0xA1", "value": "0x00" } }
  ]
}
```
