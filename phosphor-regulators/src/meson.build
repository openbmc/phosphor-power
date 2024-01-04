phosphor_regulators_include_directories = include_directories(
    '.',
    '../..',
    'actions'
)

phosphor_regulators_library_source_files = [
    'chassis.cpp',
    'config_file_parser.cpp',
    'configuration.cpp',
    'dbus_sensor.cpp',
    'dbus_sensors.cpp',
    'device.cpp',
    'error_logging.cpp',
    'error_logging_utils.cpp',
    'exception_utils.cpp',
    'ffdc_file.cpp',
    'id_map.cpp',
    'journal.cpp',
    'phase_fault_detection.cpp',
    'pmbus_utils.cpp',
    'presence_detection.cpp',
    'presence_service.cpp',
    'rail.cpp',
    'sensor_monitoring.cpp',
    'system.cpp',
    'vpd.cpp',

    'actions/compare_presence_action.cpp',
    'actions/compare_vpd_action.cpp',
    'actions/if_action.cpp',
    'actions/i2c_capture_bytes_action.cpp',
    'actions/i2c_compare_bit_action.cpp',
    'actions/i2c_compare_byte_action.cpp',
    'actions/i2c_compare_bytes_action.cpp',
    'actions/i2c_write_bit_action.cpp',
    'actions/i2c_write_byte_action.cpp',
    'actions/i2c_write_bytes_action.cpp',
    'actions/pmbus_read_sensor_action.cpp',
    'actions/pmbus_write_vout_command_action.cpp'
]

phosphor_regulators_library = static_library(
    'phosphor-regulators',
    phosphor_regulators_library_source_files,
    implicit_include_directories: false,
    dependencies: [
        nlohmann_json_dep,
        phosphor_dbus_interfaces,
        phosphor_logging,
        sdbusplus,
    ],
    include_directories: [
        phosphor_regulators_include_directories,
        libi2c_inc
    ]
)

phosphor_regulators = executable(
    'phosphor-regulators',
    'interfaces/manager_interface.cpp',
    'main.cpp',
    'manager.cpp',
    dependencies: [
        libi2c_dep,
        nlohmann_json_dep,
        phosphor_logging,
        sdbusplus,
        sdeventplus,
        stdplus,
    ],
    link_with: [
        phosphor_regulators_library,
        libpower
    ],
    implicit_include_directories: false,
    include_directories: phosphor_regulators_include_directories,
    install: true
)

regsctl = executable(
    'regsctl',
    'regsctl/main.cpp',
    dependencies: [
        cli11_dep,
        sdbusplus,
    ],
    install: true
)
