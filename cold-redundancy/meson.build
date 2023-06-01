cold_redundancy = executable(
    'cold-redundancy',
    'redundancy_main.cpp',
    'cold_redundancy.cpp',
    'util.cpp',
    dependencies: [
        boost,
        nlohmann_json_dep,
        phosphor_dbus_interfaces,
        phosphor_logging,
        pthread,
        sdbusplus,
        systemd,
    ],
    include_directories: [
        '.',
        '..',
    ],
    install: false,
)
