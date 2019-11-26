
executable(
    'phosphor-psu-monitor',
    'main.cpp',
    'psu_manager.cpp',
    'power_supply.cpp',
    dependencies: [
        sdbusplus,
        sdeventplus,
    ],
    include_directories: '..',
    install: true,
    install_dir: get_option('bindir'),
    link_with: [
        libpower,
    ]
)
