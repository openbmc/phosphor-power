psutils = executable(
    'psutils',
    'argument.cpp',
    'version.cpp',
    'main.cpp',
    dependencies: [
        phosphor_dbus_interfaces,
        phosphor_logging,
    ],
    include_directories: '..',
    install: true,
    link_with: [
        libpower,
    ]
)
