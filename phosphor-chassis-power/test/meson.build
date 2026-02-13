# Test applications
test(
    'phosphor-chassis-power-tests',
    executable(
        'phosphor-chassis-power-tests',
        'manager_tests.cpp',
        dependencies: [
            gmock,
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
