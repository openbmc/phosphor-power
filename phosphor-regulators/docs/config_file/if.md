# if

## Description

Performs actions based on whether a condition is true.

The "condition" property specifies an action to execute. The condition is true
if the action returns true. Otherwise the condition is false.

If the condition is true, the actions within the "then" property are executed.

If the condition is false, the actions within the "else" property are executed
(if specified).

## Properties

| Name      | Required | Type                          | Description                                           |
| :-------- | :------: | :---------------------------- | :---------------------------------------------------- |
| condition |   yes    | [action](action.md)           | Action that tests whether condition is true.          |
| then      |   yes    | array of [actions](action.md) | One or more actions to perform if condition is true.  |
| else      |    no    | array of [actions](action.md) | One or more actions to perform if condition is false. |

## Return Value

If the condition was true, returns the value of the last action in the "then"
property.

If the condition was false, returns the value of the last action in the "else"
property. If no "else" property was specified, returns false.

## Example

```json
{
  "comments": ["If regulator is downlevel use different configuration rule"],
  "if": {
    "condition": {
      "run_rule": "is_downlevel_regulator"
    },
    "then": [{ "run_rule": "configure_downlevel_regulator" }],
    "else": [{ "run_rule": "configure_standard_regulator" }]
  }
}
```
