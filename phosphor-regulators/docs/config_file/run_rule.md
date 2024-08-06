# run_rule

## Description

Runs the specified rule.

Note: Make sure that two rules do not run each other recursively (rule "A" runs
rule "B" which runs rule "A").

## Property Value

String containing the unique ID of the [rule](rule.md) to run.

## Return Value

Return value of the rule that was run.

## Example

```json
{
  "run_rule": "set_voltage_rule"
}
```
