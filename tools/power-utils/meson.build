psutils = executable(
    'psutils',
    'version.cpp',
    'updater.cpp',
    'main.cpp',
    dependencies: [
        phosphor_dbus_interfaces,
        phosphor_logging,
    ],
    include_directories: '../..',
    install: true,
    link_with: [
        libpower,
    ]
)

subdir('test')
