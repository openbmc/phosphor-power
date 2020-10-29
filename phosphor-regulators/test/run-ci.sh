#!/bin/bash
for file in phosphor-regulators/config_files/*
do
    /usr/bin/env python3 \
    phosphor-regulators/tools/validate-regulators-config.py \
    -s phosphor-regulators/schema/config_schema.json \
    -c $file
done
