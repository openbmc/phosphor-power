psutils = executable(
    'psutils',
    'version.cpp',
    'updater.cpp',
    'aei_updater.cpp',
    'utils.cpp',
    'model.cpp',
    'main.cpp',
    dependencies: [
        cli11_dep,
        libi2c_dep,
        nlohmann_json_dep,
        phosphor_dbus_interfaces,
        phosphor_logging,
    ],
    include_directories: [libpower_inc, libi2c_inc],
    install: true,
    link_with: [libpower],
)

if get_option('tests').allowed()
    subdir('test')
endif
