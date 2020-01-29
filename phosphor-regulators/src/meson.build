phosphor_regulators_include_directories = include_directories(
    '.',
    'actions'
)

phosphor_regulators_library_source_files = [
    'config_file_parser.cpp',
    'id_map.cpp',
    'pmbus_utils.cpp',

    'actions/if_action.cpp',
    'actions/i2c_compare_bit_action.cpp',
    'actions/i2c_compare_byte_action.cpp',
    'actions/i2c_compare_bytes_action.cpp',
    'actions/i2c_write_bit_action.cpp',
    'actions/i2c_write_byte_action.cpp',
    'actions/i2c_write_bytes_action.cpp',
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
    'journal.cpp',
    'main.cpp',
    'interfaces/manager.cpp',
    'manager.cpp',
    dependencies: [
        libi2c_dep,
        phosphor_logging,
        sdbusplus,
        sdeventplus,
        stdplus
    ],
    link_with: phosphor_regulators_library,
    implicit_include_directories: false,
    include_directories: phosphor_regulators_include_directories,
    install: true
)
