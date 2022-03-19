install_subdir('configurations',
      strip_directory: true,
      install_dir: get_option('datadir')/'phosphor-psu-monitor')

phosphor_psu_monitor = executable(
    'phosphor-psu-monitor',
    'main.cpp',
    'psu_manager.cpp',
    'power_supply.cpp',
    'util.cpp',
    dependencies: [
        sdbusplus,
        sdeventplus,
        fmt,
        libgpiodcxx,
        phosphor_dbus_interfaces,
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
