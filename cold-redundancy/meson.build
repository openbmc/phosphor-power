cold_redundancy = executable(
    'cold-redundancy',
    'redundancy_main.cpp',
    'cold_redundancy.cpp',
    'utility.cpp',
    dependencies: [
        phosphor_dbus_interfaces,
        phosphor_logging,
        sdbusplus,
        systemd,
        pthread,
    ],
    include_directories: '.',
    install: false,
)
