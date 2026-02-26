test(
    'phosphor-chassis-power-tests',
    executable(
        'phosphor-chassis-power-tests',
        'device_tests.cpp',
        '../src/device.cpp',
        'chassis_tests.cpp',
        '../src/chassis.cpp',
        'config_file_parser_tests.cpp',
        '../src/config_file_parser.cpp',
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
