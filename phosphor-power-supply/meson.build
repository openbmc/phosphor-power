install_subdir(
    'configurations',
    strip_directory: true,
    install_dir: get_option('datadir') / 'phosphor-psu-monitor',
)

phosphor_psu_monitor = executable(
    'phosphor-psu-monitor',
    'new_main.cpp',
    'chassis_manager.cpp',
    'chassis.cpp',
    'new_power_supply.cpp',
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

# The following lines are commented out until phosphor-psu-monitor is
# permanently replaced with the new code and the CI test cases are
# ready for multi/single chasssis.
#
# For now the test directory is not included in the build.
#
#phosphor_psu_monitor = executable(
#    'phosphor-psu-monitor',
#    'main.cpp',
#    'psu_manager.cpp',
#    'power_supply.cpp',
#    'util.cpp',
#    dependencies: [
#        cli11_dep,
#        libgpiodcxx,
#        nlohmann_json_dep,
#        phosphor_dbus_interfaces,
#        phosphor_logging,
#        sdbusplus,
#        sdeventplus,
#    ],
#    include_directories: '..',
#    install: true,
#    link_with: [libpower],
#)
#
#power_supply = phosphor_psu_monitor.extract_objects('power_supply.cpp')
#
#if get_option('tests').allowed()
#    subdir('test')
#endif
