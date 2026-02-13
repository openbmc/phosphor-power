# Test applications
phosphor_chassis_power_tests_source_files = [
    'manager_tests.cpp',
]
test(
    'phosphor-chassis-power-tests',
    executable(
        'phosphor-chassis-power-tests',
        phosphor_chassis_power_tests_source_files,
        dependencies: [
            gtest,
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
