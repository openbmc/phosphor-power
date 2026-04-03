#!/usr/bin/env python3

import argparse
import copy
import json
import os
import re
import sys

import jsonschema

r"""
Validates the phosphor-regulators configuration file. Checks it against a JSON
schema as well as doing some extra checks that can't be encoded in the schema.
"""

TEMPLATE_VARIABLE_PATTERN = r"\$\{[A-Za-z0-9_]+\}"


class CommandLineError(Exception):
    r"""
    Exception for command line syntax errors.
    """

    pass


class ValidationError(Exception):
    r"""
    Exception for content validation errors.
    """

    def __init__(self, message):
        super().__init__(message + "\nValidation failed.")


def get_values(json_element, key, result=None):
    r"""
    Finds all occurrences of a key within the specified JSON element and its
    children. Returns the associated values.
    To search the entire configuration file, pass the root JSON element
    json_element: JSON element within the config file.
    key: key name.
    result: list of values found with the specified key.
    """

    if result is None:
        result = []
    if type(json_element) is dict:
        for json_key in json_element:
            if json_key == key:
                result.append(json_element[json_key])
            elif type(json_element[json_key]) in (list, dict):
                get_values(json_element[json_key], key, result)
    elif type(json_element) is list:
        for item in json_element:
            if type(item) in (list, dict):
                get_values(item, key, result)
    return result


def get_rule_ids(config_json):
    r"""
    Get all rule IDs in the configuration file.
    config_json: Configuration file JSON
    """
    rule_ids = []
    for rule in config_json.get("rules", {}):
        rule_ids.append(rule["id"])
    return rule_ids


def get_device_ids(config_json):
    r"""
    Get all device IDs in the configuration file.
    config_json: Configuration file JSON
    """
    device_ids = []
    for chassis in config_json.get("chassis", {}):
        for device in chassis.get("devices", {}):
            device_ids.append(device["id"])
    return device_ids


def contains_template_variable(json_element):
    r"""
    Check if a JSON element or its descendants contain template variables.
    Template variables have the format ${variable_name}.
    json_element: JSON element to check
    Returns: True if template variables found, False otherwise
    """
    if type(json_element) is dict:
        for key, value in json_element.items():
            if type(value) is str:
                if re.search(TEMPLATE_VARIABLE_PATTERN, value):
                    return True
            elif type(value) in (list, dict):
                if contains_template_variable(value):
                    return True
    elif type(json_element) is list:
        for item in json_element:
            if type(item) is str:
                if re.search(TEMPLATE_VARIABLE_PATTERN, item):
                    return True
            elif type(item) in (list, dict):
                if contains_template_variable(item):
                    return True
    return False


def check_rules_for_template_variables(config_json):
    r"""
    Check that no rule objects or their descendants contain template variables.
    config_json: Configuration file JSON
    """
    for rule in config_json.get("rules", []):
        if contains_template_variable(rule):
            rule_id = rule.get("id", "unknown")
            raise ValidationError(
                f"Error: Rule contains template variable.\n"
                f"Rule {rule_id} or its descendants contain template variables.\n"
                f"Template variables are not allowed in rules."
            )


def check_chassis_for_template_variables(config_json):
    r"""
    Check that no chassis objects or their descendants contain template
    variables.
    config_json: Configuration file JSON
    """
    for chassis in config_json.get("chassis", []):
        if contains_template_variable(chassis):
            chassis_num = str(chassis.get("number", "unknown"))
            raise ValidationError(
                f"Error: Chassis contains template variable.\n"
                f"Chassis {chassis_num} or its descendants contain template "
                f"variables.\nTemplate variables are only allowed in "
                f"chassis_templates."
            )


def convert_string_value_to_native_type(value):
    r"""
    Convert a string value to its native type if possible.
    Attempts to convert to boolean, integer, or number.
    Returns the original string if conversion is not possible.
    value: String value to convert
    Returns: Converted value or original string
    """
    if not isinstance(value, str):
        return value

    # Try boolean conversion (case-insensitive)
    lower_value = value.lower()
    if lower_value == "true":
        return True
    elif lower_value == "false":
        return False

    # Try integer conversion
    try:
        # Check if it looks like an integer (no decimal point)
        if "." not in value:
            return int(value)
    except ValueError:
        pass

    # Try float/number conversion
    try:
        return float(value)
    except ValueError:
        pass

    # Return original string if no conversion worked
    return value


def expand_property_value(value, variable_values):
    r"""
    Expand any template variables in the specified property value. If variables
    were expanded, convert the value to the native type.
    value: Property value
    variable_values: Dictionary mapping variable names to their values
    Returns: Expanded property value (may be converted to native type)
    """
    if not isinstance(value, str):
        return value

    # Check if value contains any template variables
    if not re.search(TEMPLATE_VARIABLE_PATTERN, value):
        return value

    # Value contains template variables; do replacement
    for var_name, var_value in variable_values.items():
        pattern = r"\$\{" + re.escape(var_name) + r"\}"
        value = re.sub(pattern, var_value, value)

    # Check if any unreplaced variables remain
    if re.search(TEMPLATE_VARIABLE_PATTERN, value):
        remaining_vars = re.findall(r"\$\{([A-Za-z0-9_]+)\}", value)
        vars_str = ", ".join(set(remaining_vars))
        raise ValidationError(
            f"Error: No value specified for the following template variables: "
            f"{vars_str}"
        )

    # Original value had variables, so try to convert result to native type
    return convert_string_value_to_native_type(value)


def expand_object(obj, variable_values):
    r"""
    Expand any template variables in the specified object and its sub-objects.
    obj: JSON object (dict or list)
    variable_values: Dictionary mapping variable names to their values
    """
    if isinstance(obj, dict):
        for key in obj:
            obj[key] = expand_property_value(obj[key], variable_values)
            if isinstance(obj[key], (dict, list)):
                expand_object(obj[key], variable_values)
    elif isinstance(obj, list):
        for i in range(len(obj)):
            obj[i] = expand_property_value(obj[i], variable_values)
            if isinstance(obj[i], (dict, list)):
                expand_object(obj[i], variable_values)


def expand_chassis(chassis, templates_map):
    r"""
    Expands the specified chassis if it uses a chassis template. Returns the
    expanded chassis JSON. Returns the chassis JSON unchanged if it does not
    use a template.
    chassis: Chassis JSON object
    templates_map: Map of template ID to template object
    Returns: Expanded chassis JSON
    """
    if "template_id" not in chassis:
        return chassis

    # Find the template with the specified ID
    template_id = chassis["template_id"]
    if template_id not in templates_map:
        raise ValidationError(
            f"Error: Chassis template ID does not exist.\n"
            f"Found template_id value that specifies invalid chassis_template ID "
            f"{template_id}"
        )
    template = templates_map[template_id]

    # Get the template variable values
    variable_values = chassis.get("template_variable_values", {})

    # Create a deep copy of the template to avoid modifying the original
    expanded_chassis = copy.deepcopy(template)

    # Delete the 'id' property which only exists in chassis templates
    if "id" in expanded_chassis:
        del expanded_chassis["id"]

    # Add comments from the original chassis if present
    if "comments" in chassis:
        expanded_chassis["comments"] = chassis["comments"]

    # Expand variables in the new chassis object
    expand_object(expanded_chassis, variable_values)

    return expanded_chassis


def create_templates_map(config_json):
    r"""
    Create dictionary of chassis templates from the configuration file. Use the
    template id property as the dictionary key.
    config_json: Configuration file JSON
    Returns: dict from template ID to template object
    """
    templates_map = {}
    for template in config_json.get("chassis_templates", []):
        template_id = template["id"]
        if template_id in templates_map:
            raise ValidationError(
                f"Error: Duplicate chassis_template ID.\n"
                f"Found multiple chassis_templates with the ID {template_id}"
            )
        templates_map[template_id] = template
    return templates_map


def expand_chassis_templates(config_json):
    r"""
    Expand chassis templates in the configuration file.
    Creates a new JSON object with expanded chassis objects.
    config_json: Configuration file JSON
    Returns: Expanded configuration JSON, or unchanged JSON if no templates are
             present
    """
    # Create map from template IDs to templates, checking for duplicate IDs
    templates_map = create_templates_map(config_json)

    # If no chassis templates are present, return unchanged config JSON
    if len(templates_map) == 0:
        return config_json

    # Build the expanded configuration file JSON
    expanded_config_json = {}

    # Copy comments array if present
    if "comments" in config_json:
        expanded_config_json["comments"] = config_json["comments"]

    # Copy rules array if present
    if "rules" in config_json:
        expanded_config_json["rules"] = config_json["rules"]

    # Expand chassis array
    expanded_chassis = []
    for chassis in config_json.get("chassis", []):
        expanded_chassis.append(expand_chassis(chassis, templates_map))
    expanded_config_json["chassis"] = expanded_chassis

    return expanded_config_json


def check_number_of_elements_in_masks(config_json):
    r"""
    Check if the number of bit masks in the 'masks' property matches the number
    of byte values in the 'values' property.
    config_json: Configuration file JSON
    """

    i2c_write_bytes = get_values(config_json, "i2c_write_bytes")
    i2c_compare_bytes = get_values(config_json, "i2c_compare_bytes")

    for object in i2c_write_bytes + i2c_compare_bytes:
        if "masks" in object:
            masks = object.get("masks", [])
            values = object.get("values", [])
            if len(masks) != len(values):
                action = (
                    "i2c_write_bytes"
                    if object in i2c_write_bytes
                    else "i2c_compare_bytes"
                )
                raise ValidationError(
                    f"Error: Invalid {action} action.\n"
                    f"The masks array must have the same size as the values array. "
                    f"masks: {masks}, values: {values}."
                )


def check_rule_id_exists(config_json):
    r"""
    Check if a rule_id property specifies a rule ID that does not exist.
    config_json: Configuration file JSON
    """

    rule_ids = get_values(config_json, "rule_id")
    valid_rule_ids = get_rule_ids(config_json)
    for rule_id in rule_ids:
        if rule_id not in valid_rule_ids:
            raise ValidationError(
                f"Error: Rule ID does not exist.\n"
                f"Found rule_id value that specifies invalid rule ID {rule_id}"
            )


def check_device_id_exists(config_json):
    r"""
    Check if a device_id property specifies a device ID that does not exist.
    config_json: Configuration file JSON
    """

    device_ids = get_values(config_json, "device_id")
    valid_device_ids = get_device_ids(config_json)
    for device_id in device_ids:
        if device_id not in valid_device_ids:
            raise ValidationError(
                f"Error: Device ID does not exist.\n"
                f"Found device_id value that specifies invalid device ID {device_id}"
            )


def check_set_device_value_exists(config_json):
    r"""
    Check if a set_device action specifies a device ID that does not exist.
    config_json: Configuration file JSON
    """

    device_ids = get_values(config_json, "set_device")
    valid_device_ids = get_device_ids(config_json)
    for device_id in device_ids:
        if device_id not in valid_device_ids:
            raise ValidationError(
                f"Error: Device ID does not exist.\n"
                f"Found set_device action that specifies invalid device ID "
                f"{device_id}"
            )


def check_run_rule_value_exists(config_json):
    r"""
    Check if any run_rule actions specify a rule ID that does not exist.
    config_json: Configuration file JSON
    """

    rule_ids = get_values(config_json, "run_rule")
    valid_rule_ids = get_rule_ids(config_json)
    for rule_id in rule_ids:
        if rule_id not in valid_rule_ids:
            raise ValidationError(
                f"Error: Rule ID does not exist.\n"
                f"Found run_rule action that specifies invalid rule ID {rule_id}"
            )


def check_infinite_loops_in_rule(config_json, rule_json, call_stack=[]):
    r"""
    Check if a 'run_rule' action in the specified rule causes an
    infinite loop.
    config_json: Configuration file JSON.
    rule_json: A rule in the JSON config file.
    call_stack: Current call stack of rules.
    """

    call_stack.append(rule_json["id"])
    for action in rule_json.get("actions", {}):
        if "run_rule" in action:
            run_rule_id = action["run_rule"]
            if run_rule_id in call_stack:
                call_stack.append(run_rule_id)
                raise ValidationError(
                    f"Infinite loop caused by run_rule actions.\n{call_stack}"
                )
            else:
                for rule in config_json.get("rules", {}):
                    if rule["id"] == run_rule_id:
                        check_infinite_loops_in_rule(
                            config_json, rule, call_stack
                        )
    call_stack.pop()


def check_infinite_loops(config_json):
    r"""
    Check if rule in config file is called recursively, causing an
    infinite loop.
    config_json: Configuration file JSON
    """

    for rule in config_json.get("rules", {}):
        check_infinite_loops_in_rule(config_json, rule)


def check_duplicate_rule_id(config_json):
    r"""
    Check that there aren't any "rule" elements with the same 'id' field.
    config_json: Configuration file JSON
    """
    rule_ids = []
    for rule in config_json.get("rules", {}):
        rule_id = rule["id"]
        if rule_id in rule_ids:
            raise ValidationError(
                f"Error: Duplicate rule ID.\n"
                f"Found multiple rules with the ID {rule_id}"
            )
        else:
            rule_ids.append(rule_id)


def check_duplicate_chassis_number(config_json):
    r"""
    Check that there aren't any "chassis" elements with the same 'number'
    field.
    config_json: Configuration file JSON
    """
    numbers = []
    for chassis in config_json.get("chassis", {}):
        number = chassis["number"]
        if number in numbers:
            raise ValidationError(
                f"Error: Duplicate chassis number.\n"
                f"Found multiple chassis with the number {number}"
            )
        else:
            numbers.append(number)


def check_duplicate_device_id(config_json):
    r"""
    Check that there aren't any "devices" with the same 'id' field.
    config_json: Configuration file JSON
    """
    device_ids = []
    for chassis in config_json.get("chassis", {}):
        for device in chassis.get("devices", {}):
            device_id = device["id"]
            if device_id in device_ids:
                raise ValidationError(
                    f"Error: Duplicate device ID.\n"
                    f"Found multiple devices with the ID {device_id}"
                )
            else:
                device_ids.append(device_id)


def check_duplicate_rail_id(config_json):
    r"""
    Check that there aren't any "rails" with the same 'id' field.
    config_json: Configuration file JSON
    """
    rail_ids = []
    for chassis in config_json.get("chassis", {}):
        for device in chassis.get("devices", {}):
            for rail in device.get("rails", {}):
                rail_id = rail["id"]
                if rail_id in rail_ids:
                    raise ValidationError(
                        f"Error: Duplicate rail ID.\n"
                        f"Found multiple rails with the ID {rail_id}"
                    )
                else:
                    rail_ids.append(rail_id)


def check_for_duplicates(config_json):
    r"""
    Check for duplicate ID.
    """
    check_duplicate_rule_id(config_json)
    check_duplicate_chassis_number(config_json)
    check_duplicate_device_id(config_json)
    check_duplicate_rail_id(config_json)


def load_json(json_file):
    r"""
    Loads the JSON in the specified file.

    json_file: Path of the JSON file
    """

    with open(json_file) as file:
        json_contents = json.load(file)

    return json_contents


def is_json_file(file):
    try:
        load_json(file)
    except Exception:
        return False
    return True


def validate_config_using_schema(config_json, schema_json):
    r"""
    Validates the specified config file JSON using the specified schema JSON.

    config_json: Configuration file JSON
    schema_json: Schema file JSON
    """

    try:
        jsonschema.validate(config_json, schema_json)
    except jsonschema.ValidationError as e:
        raise ValidationError(str(e))


def parse_arguments():
    r"""
    Parse the command line arguments.
    Returns: tuple of configuration file and schema file
    """

    parser = argparse.ArgumentParser(
        description="phosphor-regulators configuration file validator"
    )

    parser.add_argument(
        "-s",
        "--schema-file",
        dest="schema_file",
        help="The phosphor-regulators schema file",
    )

    parser.add_argument(
        "-c",
        "--configuration-file",
        dest="configuration_file",
        help="The phosphor-regulators configuration file",
    )

    args = parser.parse_args()

    try:
        if not args.schema_file:
            raise CommandLineError("Error: Schema file is required.")
        if not os.path.exists(args.schema_file):
            raise CommandLineError("Error: Schema file does not exist.")
        if not os.access(args.schema_file, os.R_OK):
            raise CommandLineError("Error: Schema file is not readable.")
        if not is_json_file(args.schema_file):
            raise CommandLineError(
                "Error: Schema file is not in the JSON format."
            )
        if not args.configuration_file:
            raise CommandLineError("Error: Configuration file is required.")
        if not os.path.exists(args.configuration_file):
            raise CommandLineError("Error: Configuration file does not exist.")
        if not os.access(args.configuration_file, os.R_OK):
            raise CommandLineError(
                "Error: Configuration file is not readable."
            )
        if not is_json_file(args.configuration_file):
            raise CommandLineError(
                "Error: Configuration file is not in the JSON format."
            )
    except CommandLineError:
        parser.print_help()
        raise

    return (args.configuration_file, args.schema_file)


def main():
    try:
        # Parse command line arguments to get JSON config file and schema file
        configuration_file, schema_file = parse_arguments()

        # Load JSON in config file and schema file
        config_json = load_json(configuration_file)
        schema_json = load_json(schema_file)

        # Validate config file JSON using schema JSON
        validate_config_using_schema(config_json, schema_json)

        # Check for template variables in rules and chassis; these are not valid
        check_rules_for_template_variables(config_json)
        check_chassis_for_template_variables(config_json)

        # Expand any chassis templates in the config file. Validate expanded
        # config file JSON using the schema JSON.
        config_json = expand_chassis_templates(config_json)
        validate_config_using_schema(config_json, schema_json)

        # Check for other possible errors in the config file JSON
        check_for_duplicates(config_json)
        check_infinite_loops(config_json)
        check_run_rule_value_exists(config_json)
        check_set_device_value_exists(config_json)
        check_rule_id_exists(config_json)
        check_device_id_exists(config_json)
        check_number_of_elements_in_masks(config_json)

    except (CommandLineError, ValidationError) as e:
        sys.exit(str(e))


if __name__ == "__main__":
    main()
