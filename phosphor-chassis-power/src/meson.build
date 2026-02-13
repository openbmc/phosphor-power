phosphor_chassis_power_include_directories = include_directories('.', '../..')

phosphor_chassis_power = executable(
    'phosphor-chassis-power',
    'main.cpp',
    'manager.cpp',
    dependencies: [
        phosphor_logging,
        sdbusplus,
        sdeventplus,
    ],
    link_with: [
        libpower,
    ],
    implicit_include_directories: false,
    install: true,
    install_dir: get_option('libexecdir') / 'phosphor-power',
    include_directories: [
        libpower_inc,
        phosphor_chassis_power_include_directories,
    ],
)
