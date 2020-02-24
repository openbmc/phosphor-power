phosphor_regulators_include_directories = include_directories(
    '.',
    'actions'
)

phosphor_regulators_library_source_files = [
    'id_map.cpp',

    'actions/if_action.cpp',
    'actions/i2c_compare_bit_action.cpp',
    'actions/i2c_compare_byte_action.cpp',
    'actions/i2c_compare_bytes_action.cpp',
    'actions/i2c_write_byte_action.cpp'
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
    'main.cpp',
    dependencies: [
        sdbusplus,
        sdeventplus,
        libi2c_dep
    ],
    link_with: phosphor_regulators_library,
    implicit_include_directories: false,
    include_directories: phosphor_regulators_include_directories,
    install: true
)
