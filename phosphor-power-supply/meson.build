install_subdir(
    'configurations',
    strip_directory: true,
    install_dir: get_option('datadir') / 'phosphor-psu-monitor',
)

phosphor_psu_monitor = executable(
    'phosphor-psu-monitor',
    'main.cpp',
    'chassis_manager.cpp',
    'chassis.cpp',
    'power_supply.cpp',
    'util.cpp',
    dependencies: [
        cli11_dep,
        libgpiodcxx,
        nlohmann_json_dep,
        phosphor_dbus_interfaces,
        phosphor_logging,
        sdbusplus,
        sdeventplus,
    ],
    include_directories: '..',
    install: true,
    link_with: [libpower],
)
