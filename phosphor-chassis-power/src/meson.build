phosphor_chassis_power_include_directories = include_directories('.', '../..')

phosphor_chassis_power_lib = static_library(
    'phosphor-chassis-power',
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
    'chassis.cpp',
    'gpio.cpp',
    'config_file_parser.cpp',
    dependencies: [sdbusplus],
    link_with: [libpower, phosphor_chassis_power_lib],
    implicit_include_directories: false,
    install: true,
    install_dir: get_option('libexecdir') / 'phosphor-power',
    include_directories: [
        libpower_inc,
        phosphor_chassis_power_include_directories,
    ],
)
