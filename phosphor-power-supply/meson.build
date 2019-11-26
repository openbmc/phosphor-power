
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
    link_with: [
        libpower,
    ]
)
