# Test applications
test(
    'chassis-tests',
    executable(
        'chassis-tests',
        'manager_test.cpp',
        '../src/manager.cpp',
        dependencies: [
            gmock,
            gtest,
            phosphor_logging,
            sdbusplus,
            sdeventplus,
        ],
        link_with: [
            libpower,
        ],
        implicit_include_directories: false,
        include_directories: [
            libpower_inc,
            phosphor_chassis_power_include_directories,
        ],
    ),
)
