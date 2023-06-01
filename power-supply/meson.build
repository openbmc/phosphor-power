psu_monitor = executable(
    'psu-monitor',
    'argument.cpp',
    error_hpp,
    'main.cpp',
    'power_supply.cpp',
    'record_manager.cpp',
    dependencies: [
        nlohmann_json_dep,
        phosphor_dbus_interfaces,
        phosphor_logging,
        sdbusplus,
        sdeventplus,
    ],
    include_directories: '..',
    install: true,
    link_with: [
        libpower,
    ]
)

record_manager = psu_monitor.extract_objects('record_manager.cpp')

if get_option('tests').enabled()
    subdir('test')
endif
