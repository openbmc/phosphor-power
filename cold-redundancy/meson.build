cold_redundancy = executable(
    'cold-redundancy',
    'redundancy_main.cpp',
    'cold_redundancy.cpp',
    'util.cpp',
    dependencies: [
        phosphor_dbus_interfaces,
        phosphor_logging,
        sdbusplus,
        systemd,
        pthread,
        boost,
    ],
    include_directories: [
        '.',
        '..',
    ],
    install: false,
)
