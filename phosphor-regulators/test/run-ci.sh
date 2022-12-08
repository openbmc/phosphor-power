#!/bin/bash
rc=0
for file in phosphor-regulators/config_files/*.json
do
    phosphor-regulators/tools/validate-regulators-config.py \
        -s phosphor-regulators/schema/config_schema.json \
        -c "$file" || rc=1
done
exit $rc
