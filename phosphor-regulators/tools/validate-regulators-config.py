#!/usr/bin/env python

import argparse
import json
import sys
import jsonschema

r"""
Validates the phosphor-regulators configuration file. Checks it against a JSON
schema as well as doing some extra checks that can't be encoded in the schema.
"""

def check_duplicate_rule_id(config_json):
    r"""
    Check that there aren't any "rule" entries with the same 'id' field.
    config_json: The patform config JSON
    """
    ruleID = {}
    if 'rules' in config_json:
        for entry in config_json['rules']:
            if entry['id'] in ruleID.keys():
                print("Found multiple rule ID of error {}".format(entry['id']))
                sys.exit("Duplicate ID failed.")
            else:
                ruleID[entry['id']] = {}

def check_duplicate_chassis_number(config_json):
    r"""
    Check that there aren't any "chassis" entries with the same'device' field.
    config_json: The patform config JSON
    """
    number = {}
    if 'chassis' in config_json:
        for entry in config_json['chassis']:
            if entry['number'] in number.keys():
                print("Found multiple chassis number of error {}"\
                .format(entry['number']))
                sys.exit("Duplicate number failed.")
            else:
                number[entry['number']] = {}

def check_duplicate_device_id(config_json):
    r"""
    Check that there aren't any "chassis/devices" entries with the same 'id'
    field.
    config_json: The patform config JSON
    """
    deviceID = {}
    if 'chassis' in config_json:
        for entry in config_json['chassis']:
            if 'devices' in entry:
                for entry in entry['devices']:
                    if entry['id'] in deviceID.keys():
                        print("Found multiple devices ID of error {}"\
                        .format(entry['id']))
                        sys.exit("Duplicate ID failed.")
                    else:
                        deviceID[entry['id']] = {}

def check_duplicate_rail_id(config_json):
    r"""
    Check that there aren't any "chassis/devices/rails" entries with the same
    'id' field.
    config_json: The patform config JSON
    """
    railID = {}
    if 'chassis' in config_json:
        for entry in config_json['chassis']:
            if 'devices' in entry:
                for entry in entry['devices']:
                    if 'rails' in entry:
                        for entry in entry['rails']:
                            if entry['id'] in railID.keys():
                                print("Found multiple rails ID of error {}"\
                                .format(entry['id']))
                                sys.exit("Duplicate ID failed.")
                            else:
                                railID[entry['id']] = {}

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
                sys.exit("Validation failed.")

        check_duplicate_rule_id(config_json)

        check_duplicate_chassis_number(config_json)

        check_duplicate_device_id(config_json)

        check_duplicate_rail_id(config_json)

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

    validate_schema(args.configuration_file, args.schema_file)
