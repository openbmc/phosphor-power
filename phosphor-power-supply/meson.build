install_subdir('configurations',
      strip_directory: true,
      install_dir: get_option('datadir')/'phosphor-psu-monitor')

phosphor_psu_monitor = executable(
    'phosphor-psu-monitor',
    'main.cpp',
    'psu_manager.cpp',
    'power_supply.cpp',
    'record_manager.cpp',
    'util.cpp',
    dependencies: [
        cli11_dep,
        fmt,
        libgpiodcxx,
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

power_supply = phosphor_psu_monitor.extract_objects('power_supply.cpp')

if get_option('tests').enabled()
  subdir('test')
endif
