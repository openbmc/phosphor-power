# Test applications
test(
    'chassis-tests',
    executable(
        'chassis-tests',
        'chassis_tests.cpp',
        dependencies: [
            gtest_dep,
            gmock_dep,
            phosphor_dbus_interfaces_dep,
            phosphor_logging_dep,
            sdbusplus_dep,
            sdeventplus_dep,
        ],
        link_with: [
            phosphor_chassis_power_lib,
        ],
        include_directories: '../src',
    ),
)
