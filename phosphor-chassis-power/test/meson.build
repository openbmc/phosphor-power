phosphor_chassis_power_include_directories = include_directories(
    '.',
    '../src/',
)

phosphor_chassis_power_tests_source_files = [
    'gpio_tests.cpp',
    '../src/gpio.cpp',
    'chassis_tests.cpp',
    '../src/chassis.cpp',
    'config_file_parser_tests.cpp',
    '../src/config_file_parser.cpp',
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
        link_with: [libpower],
        implicit_include_directories: false,
        include_directories: [
            libpower_inc,
            phosphor_chassis_power_include_directories,
        ],
    ),
)
