phosphor_regulators_include_directories = include_directories(
    '.',
    '../..',
    'actions'
)

phosphor_regulators_library_source_files = [
    'chassis.cpp',
    'config_file_parser.cpp',
    'configuration.cpp',
    'device.cpp',
    'exception_utils.cpp',
    'id_map.cpp',
    'pmbus_utils.cpp',
    'rail.cpp',
    'sensor_monitoring.cpp',
    'system.cpp',

    'actions/if_action.cpp',
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
        phosphor_logging,
        sdbusplus,
        sdeventplus,
        stdplus
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
        sdbusplus
    ],
    install: true
)
