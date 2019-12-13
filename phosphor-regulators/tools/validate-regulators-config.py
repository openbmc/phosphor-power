#!/usr/bin/env python

import argparse
import json
import sys

r"""
Validates the phosphor-power/phosphor-regulators config JSON, which 
includes checking it against a JSON schema using the jsonschema module as 
well as doing some extra checks that can't be encoded in the schema.
"""

def validate_schema(config, schema):
    r"""
    Validates the passed in JSON against the passed in schema JSON

    config:   Path of the file containing the config JSON
    schema:   Path of the file containing the schema JSON
              Use None to skip the pure schema validation
    """

    with open(config) as config_handle:
        config_json = json.load(config_handle)

        if schema:

            import jsonschema

            with open(schema) as schema_handle:
                schema_json = json.load(schema_handle)

                try:
                    jsonschema.validate(config_json, schema_json)
                except jsonschema.ValidationError as e:
                    print(e)
                    sys.exit("Schema validation failed")

if __name__ == '__main__':

    parser = argparse.ArgumentParser(
        description='validate phosphor-regulators configuration file')

    parser.add_argument('-s', '--schema-file', dest='schema_file',
                        help='The power regulators JSON schema file')

    parser.add_argument('-c', '--configuration-file', dest='configuration_file',
                        help='The power regulators configuration JSON file')
    parser.add_argument('-k', '--skip-schema-validation', action='store_true',
                        dest='skip_schema',
                        help='Skip running schema validation. '
                             'Only do the extra checks.')

    args = parser.parse_args()

    if not args.schema_file:
        sys.exit("Schema file required")

    if not args.configuration_file:
        sys.exit("configuration file required")

    schema = args.schema_file
    if args.skip_schema:
        schema = None

    validate_schema(args.configuration_file, schema)

    sys.exit()
