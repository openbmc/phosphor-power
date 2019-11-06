psutils = executable(
    'psutils',
    'version.cpp',
    'updater.cpp',
    'main.cpp',
    dependencies: [
        phosphor_dbus_interfaces,
        phosphor_logging,
        libi2c_dep,
    ],
    include_directories: [libpower_inc, libi2c_inc],
    install: true,
    link_with: [
        libpower,
    ]
)

subdir('test')
