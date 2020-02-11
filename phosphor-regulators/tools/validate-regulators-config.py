#!/usr/bin/env python

import argparse
import json
import sys
import jsonschema

r"""
Validates the phosphor-regulators configuration file. Checks it against a JSON
schema as well as doing some extra checks that can't be encoded in the schema.
"""

def handle_validation_error():
    sys.exit("Validation failed.")

def get_value(config_json, key, result = []):
    r"""
    find the value of key in config file, store in result list and return
    result.
    config_json: Configuration file JSON
    key: the key to get.
    result: list to store the value of the key.
    """

    if type(config_json) == str:
        config_json = json.loads(config_json)
    if type(config_json) is dict:
        for json_key in config_json:
            if type(config_json[json_key]) in (list, dict):
                get_value(config_json[json_key], key, result)
            elif json_key == key:
                result.append(config_json[json_key])
    elif type(config_json) is list:
        for item in config_json:
            if type(item) in (list, dict):
                get_value(item, key, result)
    return result

def check_rule_id_exist(config_json):
    r"""
    Check if a rule_id property specifies a rule ID that does not exist.
    config_json: Configuration file JSON
    """

    rule_id = get_value(config_json, 'rule_id', [])
    for rule_id_value in rule_id:
        if rule_id_value not in check_duplicate_rule_id(config_json):
            sys.stderr.write("Error: rule_id not exist.\n"+\
            "Found rule_id value not exist "+rule_id_value+'\n')
            handle_validation_error()

def check_set_device_value_exist(config_json):
    r"""
    Check if a set_device action specifies a device ID that does not exist.
    config_json: Configuration file JSON
    """

    set_device = get_value(config_json, 'set_device', [])
    for set_device_key in set_device:
        if set_device_key not in check_duplicate_device_id(config_json):
            sys.stderr.write("Error: set_device not exist.\n"+\
            "Found set_device value not exist "+set_device_key+'\n')
            handle_validation_error()

def check_run_rule_value_exist(config_json):
    r"""
    Check if any run_rule actions specify a rule ID that does not exist.
    config_json: Configuration file JSON
    """

    run_rule = get_value(config_json, 'run_rule', [])
    for run_rule_key in run_rule:
        if run_rule_key not in check_duplicate_rule_id(config_json):
            sys.stderr.write("Error: run_rule not exist.\n"+\
            "Found run_rule value not exist "+run_rule_key+'\n')
            handle_validation_error()

def check_infinite_loops(config_json):
    r"""
    Check if rule in config file called recursion and casue infinite loop.
    Check 'rule' object and call check_infinite_loops_for_run_rule.
    config_json: Configuration file JSON
    """

    def check_infinite_loops_for_run_rule(config_json, rule_json, \
        callStack=[]):
        r"""
        Check if 'run_rule' call rule recursion and cause infinite loop
        config_json: Original platform config JSON.
        rule_json: Configuration file JSON which be excuted.
        callStack: Current call stack of rules.
        """

        callStack.append(rule_json['id'])
        for action in rule_json.get('actions', {}):
            if 'run_rule' in action:
                run_rule_id = action['run_rule']
                if run_rule_id in callStack:
                    callStack.append(run_rule_id)
                    sys.stderr.write(\
                   "Infinite loop caused by run_rule actions.\n"+\
                    str(callStack)+'\n')
                    handle_validation_error()
                else:
                    for rule in config_json['rules']:
                        if rule['id'] == action.get('run_rule', {}):
                            check_infinite_loops_for_run_rule(\
                            config_json, rule, callStack)
        callStack.pop()
        # end of function check_infinite_loops_for_run_rule.
    for rule in config_json.get('rules', {}):
        check_infinite_loops_for_run_rule(config_json, rule)

def check_duplicate_object_id(config_json):
    r"""
    Check that there aren't any JSON objects with the same 'id' property value.
    config_json: Configuration file JSON
    """

    JSON_ids = []
    JSON_ids = check_duplicate_rule_id(config_json)+\
               check_duplicate_device_id(config_json)+\
               check_duplicate_rail_id(config_json)
    JSON_set = set()
    for ids in JSON_ids:
        if ids in JSON_set:
           sys.stderr.write("Error: Duplicate ID.\n"+\
           "Found multiple objects with the ID "+ids+'\n')
           handle_validation_error()
        else:
            JSON_set.add(ids)

def check_duplicate_rule_id(config_json):
    r"""
    Check that there aren't any "rule" elements with the same 'id' field.
    config_json: Configuration file JSON
    """
    rule_ids = []
    for rule in config_json.get('rules', {}):
        rule_id = rule['id']
        if rule_id in rule_ids:
            sys.stderr.write("Error: Duplicate rule ID.\n"+\
            "Found multiple rules with the ID "+rule_id+'\n')
            handle_validation_error()
        else:
            rule_ids.append(rule_id)
    return rule_ids

def check_duplicate_chassis_number(config_json):
    r"""
    Check that there aren't any "chassis" elements with the same 'number' field.
    config_json: Configuration file JSON
    """
    numbers = []
    for chassis in config_json.get('chassis', {}):
        number = chassis['number']
        if number in numbers:
            sys.stderr.write("Error: Duplicate chassis number.\n"+\
            "Found multiple chassis with the number "+str(number)+'\n')
            handle_validation_error()
        else:
            numbers.append(number)

def check_duplicate_device_id(config_json):
    r"""
    Check that there aren't any "devices" with the same 'id' field.
    config_json: Configuration file JSON
    """
    device_ids = []
    for chassis in config_json.get('chassis', {}):
        for device in chassis.get('devices', {}):
            device_id = device['id']
            if device_id in device_ids:
                sys.stderr.write("Error: Duplicate device ID.\n"+\
                "Found multiple devices with the ID "+device_id+'\n')
                handle_validation_error()
            else:
                device_ids.append(device_id)
    return device_ids

def check_duplicate_rail_id(config_json):
    r"""
    Check that there aren't any "rails" with the same 'id' field.
    config_json: Configuration file JSON
    """
    rail_ids = []
    for chassis in config_json.get('chassis', {}):
        for device in chassis.get('devices', {}):
            for rail in device.get('rails', {}):
                rail_id = rail['id']
                if rail_id in rail_ids:
                    sys.stderr.write("Error: Duplicate rail ID.\n"+\
                    "Found multiple rails with the ID "+rail_id+'\n')
                    handle_validation_error()
                else:
                    rail_ids.append(rail_id)
    return rail_ids

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

if __name__ == '__main__':

    parser = argparse.ArgumentParser(
        description='phosphor-regulators configuration file validator')

    parser.add_argument('-s', '--schema-file', dest='schema_file',
                        help='The phosphor-regulators schema file')

    parser.add_argument('-c', '--configuration-file', dest='configuration_file',
                        help='The phosphor-regulators configuration file')

    args = parser.parse_args()

    if not args.schema_file:
        parser.print_help()
        sys.exit("Error: Schema file is required.")
    if not args.configuration_file:
        parser.print_help()
        sys.exit("Error: Configuration file is required.")

    config_json = validate_schema(args.configuration_file, args.schema_file)

    check_for_duplicates(config_json)

    check_infinite_loops(config_json)

    check_run_rule_value_exist(config_json)

    check_set_device_value_exist(config_json)

    check_rule_id_exist(config_json)
