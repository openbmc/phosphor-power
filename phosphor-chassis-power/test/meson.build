phosphor_chassis_power_include_directories = include_directories(
    '.',
    '../src/',
)

phosphor_chassis_power_tests_source_files = [
    'gpio_tests.cpp',
    'chassis_tests.cpp',
    'config_file_parser_tests.cpp',
]

test(
    'phosphor-chassis-power-tests',
    executable(
        'phosphor-chassis-power-tests',
        phosphor_chassis_power_tests_source_files,
        dependencies: [
            gtest,
            nlohmann_json_dep,
            phosphor_logging,
            sdbusplus,
            sdeventplus,
        ],
        link_with: [
            libpower,
            phosphor_chassis_power_library,
        ],
        implicit_include_directories: false,
        include_directories: [
            libpower_inc,
            phosphor_chassis_power_include_directories,
        ],
    ),
)
