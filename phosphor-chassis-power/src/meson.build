phosphor_chassis_power_include_directories = include_directories('.', '../..')

phosphor_chassis_power_library = static_library(
    'phosphor-power-chassis-library',
    'manager.cpp',
    'chassis.cpp',
    'chassis_power_system_interface.cpp',
    'config_file_parser.cpp',
    'system.cpp',
    implicit_include_directories: false,
    dependencies: [
        phosphor_dbus_interfaces,
        phosphor_logging,
        sdbusplus,
        sdeventplus,
    ],
    include_directories: [phosphor_chassis_power_include_directories],
)

phosphor_chassis_power = executable(
    'phosphor-chassis-power',
    'main.cpp',
    'gpio.cpp',
    dependencies: [
        libgpiodcxx,
        phosphor_logging,
        sdbusplus,
        sdeventplus,
    ],
    link_with: [
        libpower,
        phosphor_chassis_power_library,
    ],
    implicit_include_directories: false,
    install: true,
    install_dir: get_option('libexecdir') / 'phosphor-power',
    include_directories: [
        libpower_inc,
        phosphor_chassis_power_include_directories,
    ],
)
