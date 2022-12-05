#!/usr/bin/env python3

import argparse
import json
import os
import sys

import jsonschema

r"""
Validates the phosphor-regulators configuration file. Checks it against a JSON
schema as well as doing some extra checks that can't be encoded in the schema.
"""


def handle_validation_error():
    sys.exit("Validation failed.")


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


def check_number_of_elements_in_masks(config_json):
    r"""
    Check if the number of bit masks in the 'masks' property matches the number
    of byte values in the 'values' property.
    config_json: Configuration file JSON
    """

    i2c_write_bytes = get_values(config_json, "i2c_write_bytes")
    i2c_compare_bytes = get_values(config_json, "i2c_compare_bytes")

    for object in i2c_write_bytes:
        if "masks" in object:
            if len(object.get("masks", [])) != len(object.get("values", [])):
                sys.stderr.write(
                    "Error: Invalid i2c_write_bytes action.\n"
                    + "The masks array must have the same size as the values"
                    + " array. masks: "
                    + str(object.get("masks", []))
                    + ", values: "
                    + str(object.get("values", []))
                    + ".\n"
                )
                handle_validation_error()

    for object in i2c_compare_bytes:
        if "masks" in object:
            if len(object.get("masks", [])) != len(object.get("values", [])):
                sys.stderr.write(
                    "Error: Invalid i2c_compare_bytes action.\n"
                    + "The masks array must have the same size as the values "
                    + "array. masks: "
                    + str(object.get("masks", []))
                    + ", values: "
                    + str(object.get("values", []))
                    + ".\n"
                )
                handle_validation_error()


def check_rule_id_exists(config_json):
    r"""
    Check if a rule_id property specifies a rule ID that does not exist.
    config_json: Configuration file JSON
    """

    rule_ids = get_values(config_json, "rule_id")
    valid_rule_ids = get_rule_ids(config_json)
    for rule_id in rule_ids:
        if rule_id not in valid_rule_ids:
            sys.stderr.write(
                "Error: Rule ID does not exist.\n"
                + "Found rule_id value that specifies invalid rule ID "
                + rule_id
                + "\n"
            )
            handle_validation_error()


def check_device_id_exists(config_json):
    r"""
    Check if a device_id property specifies a device ID that does not exist.
    config_json: Configuration file JSON
    """

    device_ids = get_values(config_json, "device_id")
    valid_device_ids = get_device_ids(config_json)
    for device_id in device_ids:
        if device_id not in valid_device_ids:
            sys.stderr.write(
                "Error: Device ID does not exist.\n"
                + "Found device_id value that specifies invalid device ID "
                + device_id
                + "\n"
            )
            handle_validation_error()


def check_set_device_value_exists(config_json):
    r"""
    Check if a set_device action specifies a device ID that does not exist.
    config_json: Configuration file JSON
    """

    device_ids = get_values(config_json, "set_device")
    valid_device_ids = get_device_ids(config_json)
    for device_id in device_ids:
        if device_id not in valid_device_ids:
            sys.stderr.write(
                "Error: Device ID does not exist.\n"
                + "Found set_device action that specifies invalid device ID "
                + device_id
                + "\n"
            )
            handle_validation_error()


def check_run_rule_value_exists(config_json):
    r"""
    Check if any run_rule actions specify a rule ID that does not exist.
    config_json: Configuration file JSON
    """

    rule_ids = get_values(config_json, "run_rule")
    valid_rule_ids = get_rule_ids(config_json)
    for rule_id in rule_ids:
        if rule_id not in valid_rule_ids:
            sys.stderr.write(
                "Error: Rule ID does not exist.\n"
                + "Found run_rule action that specifies invalid rule ID "
                + rule_id
                + "\n"
            )
            handle_validation_error()


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
                sys.stderr.write(
                    "Infinite loop caused by run_rule actions.\n"
                    + str(call_stack)
                    + "\n"
                )
                handle_validation_error()
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


def check_duplicate_object_id(config_json):
    r"""
    Check that there aren't any JSON objects with the same 'id' property value.
    config_json: Configuration file JSON
    """

    json_ids = get_values(config_json, "id")
    unique_ids = set()
    for id in json_ids:
        if id in unique_ids:
            sys.stderr.write(
                "Error: Duplicate ID.\n"
                + "Found multiple objects with the ID "
                + id
                + "\n"
            )
            handle_validation_error()
        else:
            unique_ids.add(id)


def check_duplicate_rule_id(config_json):
    r"""
    Check that there aren't any "rule" elements with the same 'id' field.
    config_json: Configuration file JSON
    """
    rule_ids = []
    for rule in config_json.get("rules", {}):
        rule_id = rule["id"]
        if rule_id in rule_ids:
            sys.stderr.write(
                "Error: Duplicate rule ID.\n"
                + "Found multiple rules with the ID "
                + rule_id
                + "\n"
            )
            handle_validation_error()
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
            sys.stderr.write(
                "Error: Duplicate chassis number.\n"
                + "Found multiple chassis with the number "
                + str(number)
                + "\n"
            )
            handle_validation_error()
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
                sys.stderr.write(
                    "Error: Duplicate device ID.\n"
                    + "Found multiple devices with the ID "
                    + device_id
                    + "\n"
                )
                handle_validation_error()
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
                    sys.stderr.write(
                        "Error: Duplicate rail ID.\n"
                        + "Found multiple rails with the ID "
                        + rail_id
                        + "\n"
                    )
                    handle_validation_error()
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

    check_duplicate_object_id(config_json)


def validate_schema(config, schema):
    r"""
    Validates the specified config file using the specified
    schema file.

    config:   Path of the file containing the config JSON
    schema:   Path of the file containing the schema JSON
    """

    with open(config) as config_handle:
        config_json = json.load(config_handle)

        with open(schema) as schema_handle:
            schema_json = json.load(schema_handle)

            try:
                jsonschema.validate(config_json, schema_json)
            except jsonschema.ValidationError as e:
                print(e)
                handle_validation_error()

    return config_json


def validate_JSON_format(file):
    with open(file) as json_data:
        try:
            return json.load(json_data)
        except ValueError:
            return False
        return True


if __name__ == "__main__":
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

    if not args.schema_file:
        parser.print_help()
        sys.exit("Error: Schema file is required.")
    if not os.path.exists(args.schema_file):
        parser.print_help()
        sys.exit("Error: Schema file does not exist.")
    if not os.access(args.schema_file, os.R_OK):
        parser.print_help()
        sys.exit("Error: Schema file is not readable.")
    if not validate_JSON_format(args.schema_file):
        parser.print_help()
        sys.exit("Error: Schema file is not in the JSON format.")
    if not args.configuration_file:
        parser.print_help()
        sys.exit("Error: Configuration file is required.")
    if not os.path.exists(args.configuration_file):
        parser.print_help()
        sys.exit("Error: Configuration file does not exist.")
    if not os.access(args.configuration_file, os.R_OK):
        parser.print_help()
        sys.exit("Error: Configuration file is not readable.")
    if not validate_JSON_format(args.configuration_file):
        parser.print_help()
        sys.exit("Error: Configuration file is not in the JSON format.")

    config_json = validate_schema(args.configuration_file, args.schema_file)

    check_for_duplicates(config_json)

    check_infinite_loops(config_json)

    check_run_rule_value_exists(config_json)

    check_set_device_value_exists(config_json)

    check_rule_id_exists(config_json)

    check_device_id_exists(config_json)

    check_number_of_elements_in_masks(config_json)
