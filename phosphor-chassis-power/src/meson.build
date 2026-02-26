phosphor_chassis_power_include_directories = include_directories('.', '../..')

phosphor_chassis_power_library = static_library(
    'phosphor-chassis-power',
    'chassis.cpp',
    'gpio.cpp',
    'config_file_parser.cpp',
    implicit_include_directories: false,
    dependencies: [phosphor_logging, sdbusplus, sdeventplus],
    link_with: [libpower],
    include_directories: [
        libpower_inc,
        phosphor_chassis_power_include_directories,
    ],
)

phosphor_chassis_power = executable(
    'phosphor-chassis-power',
    'main.cpp',
    dependencies: [sdbusplus],
    link_with: [libpower, phosphor_chassis_power_library],
    implicit_include_directories: false,
    install: true,
    install_dir: get_option('libexecdir') / 'phosphor-power',
    include_directories: [
        libpower_inc,
        phosphor_chassis_power_include_directories,
    ],
)
