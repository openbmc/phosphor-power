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

def check_set_device_value_exist(config_json):
    r"""
    Check if set_device value exist in rule id.
    config_json: Configuration file JSON
    """
    ids = []
    for chassis in config_json.get('chassis', {}):
        for device in chassis.get('devices', {}):
            ids.append(device['id'])
    for rule in config_json.get('rules', {}):
        for action in rule.get('actions', {}):
            if 'set_device' in action:
                set_device_value = action['set_device']
                if set_device_value not in ids:
                    sys.stderr.write("Error: set_device not exist.\n"+\
                    "Found set_device value not exist "+set_device_value+'\n')
                    handle_validation_error()

def check_run_rule_value_exist(config_json):
    r"""
    Check if run_rule value exist in rule id.
    config_json: Configuration file JSON
    """
    ids = []
    for rule in config_json.get('rules', {}):
        ids.append(rule['id'])
    for rule in config_json.get('rules', {}):
        for action in rule.get('actions', {}):
            if 'run_rule' in action:
                run_rule_value = action['run_rule']
                if run_rule_value not in ids:
                    sys.stderr.write("Error: run_rule not exist.\n"+\
                    "Found run_rule value not exist "+run_rule_value+'\n')
                    handle_validation_error()

def check_infinite_loops(config_json, callStack=[]):
    r"""
    Check if rule on call stack multiple times indicating infinite recursion.
    Check 'rule' object and call check_infinite_loops_for_run_rule.
    config_json: Configuration file JSON
    callStack: Current call stack of rules.
    """

    def check_infinite_loops_for_run_rule(config_json_origin, config_json, \
    callStack=[]):
        r"""
        Check if 'run_rule' on call stack multiple times indicating infinite 
        recursion
        config_json_origin: Original platform config JSON.
        config_json: Configuration file JSON which be excuted.
        callStack: Current call stack of rules.
        """

        callStack.append(config_json['id'])
        for action in config_json.get('actions', {}):
            run_rule_id = action.get('run_rule', {})
            if run_rule_id in callStack:
                sys.stderr.write("Error: Infinite loops.\n"+\
                "Found rule called multiple times "+run_rule_id+'\n')
                handle_validation_error()
            else:
                for rule in config_json_origin['rules']:
                    if rule['id'] == action.get('run_rule', {}):
                        check_infinite_loops_for_run_rule(\
                        config_json_origin, rule, callStack)
        callStack[:] = []
        # end of function check_infinite_loops_for_run_rule.

    for rule in config_json.get('rules', {}):
        check_infinite_loops_for_run_rule(config_json, rule, callStack)

def check_duplicate_global_id(config_json):
    r"""
    Check that there aren't any elements with the same 'id' field.
    config_json: Configuration file JSON
    """

    ids = []
    for rule in config_json.get('rules', {}):
        ids.append(rule['id'])

    for chassis in config_json.get('chassis', {}):
        for chassis in chassis.get('devices', {}):
            chassis_id = chassis['id']
            if chassis_id in ids:
                sys.stderr.write("Error: Duplicate global ID.\n"+\
                "Found multiple global with the ID "+chassis_id+'\n')
                handle_validation_error()
            else:
                ids.append(chassis_id)
            for rail in chassis.get('rails', {}):
                rail_id = rail['id']
                if rail_id in ids:
                    sys.stderr.write("Error: Duplicate global ID.\n"+\
                    "Found multiple global with the ID "+rail_id+'\n')
                    handle_validation_error()
                else:
                    ids.append(rail_id)

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

def check_for_duplicates(config_json):
    r"""
    Check for duplicate ID.
    """
    check_duplicate_rule_id(config_json)

    check_duplicate_chassis_number(config_json)

    check_duplicate_device_id(config_json)

    check_duplicate_rail_id(config_json)

    check_duplicate_global_id(config_json)

    check_infinite_loops(config_json)

    check_run_rule_value_exist(config_json)

    check_set_device_value_exist(config_json)

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
