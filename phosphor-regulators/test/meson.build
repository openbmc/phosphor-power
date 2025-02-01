phosphor_regulators_tests_include_directories = include_directories(
    '.',
    'actions',
)

phosphor_regulators_tests_source_files = [
    'chassis_tests.cpp',
    'config_file_parser_error_tests.cpp',
    'config_file_parser_tests.cpp',
    'configuration_tests.cpp',
    'device_tests.cpp',
    'error_history_tests.cpp',
    'error_logging_utils_tests.cpp',
    'exception_utils_tests.cpp',
    'ffdc_file_tests.cpp',
    'id_map_tests.cpp',
    'phase_fault_detection_tests.cpp',
    'phase_fault_tests.cpp',
    'pmbus_error_tests.cpp',
    'pmbus_utils_tests.cpp',
    'presence_detection_tests.cpp',
    'rail_tests.cpp',
    'rule_tests.cpp',
    'sensor_monitoring_tests.cpp',
    'sensors_tests.cpp',
    'system_tests.cpp',
    'write_verification_error_tests.cpp',
    'actions/action_environment_tests.cpp',
    'actions/action_error_tests.cpp',
    'actions/action_utils_tests.cpp',
    'actions/and_action_tests.cpp',
    'actions/compare_presence_action_tests.cpp',
    'actions/compare_vpd_action_tests.cpp',
    'actions/i2c_action_tests.cpp',
    'actions/i2c_capture_bytes_action_tests.cpp',
    'actions/i2c_compare_bit_action_tests.cpp',
    'actions/i2c_compare_byte_action_tests.cpp',
    'actions/i2c_compare_bytes_action_tests.cpp',
    'actions/i2c_write_bit_action_tests.cpp',
    'actions/i2c_write_byte_action_tests.cpp',
    'actions/i2c_write_bytes_action_tests.cpp',
    'actions/if_action_tests.cpp',
    'actions/log_phase_fault_action_tests.cpp',
    'actions/not_action_tests.cpp',
    'actions/or_action_tests.cpp',
    'actions/pmbus_read_sensor_action_tests.cpp',
    'actions/pmbus_write_vout_command_action_tests.cpp',
    'actions/run_rule_action_tests.cpp',
    'actions/set_device_action_tests.cpp',
]

# Long-running tests that are excluded from CI
if get_option('long-tests').allowed()
    # This test requires the Python module 'jsonschema'
    phosphor_regulators_tests_source_files += 'validate-regulators-config_tests.cpp'
endif

test(
    'phosphor-regulators-tests',
    executable(
        'phosphor-regulators-tests',
        phosphor_regulators_tests_source_files,
        dependencies: [
            gmock,
            gtest,
            nlohmann_json_dep,
            phosphor_dbus_interfaces,
            phosphor_logging,
            sdbusplus,
        ],
        link_args: dynamic_linker,
        build_rpath: get_option('oe-sdk').allowed() ? rpath : '',
        link_with: [phosphor_regulators_library, libi2c_dev_mock, libpower],
        implicit_include_directories: false,
        include_directories: [
            phosphor_regulators_include_directories,
            phosphor_regulators_tests_include_directories,
            libi2c_inc,
            libi2c_dev_mock_inc,
        ],
    ),
    timeout: 180,
)
