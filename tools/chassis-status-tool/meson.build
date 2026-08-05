chassisstatustool = executable(
    'chassis-status-tool',
    'chassis-status-tool.cpp',
    'pldm-fetcher.cpp',
    dependencies: [
        cli11_dep,
        libpldm_dep,
        phosphor_dbus_interfaces,
        phosphor_logging,
    ],
    include_directories: [libpower_inc],
    install: true,
    link_with: [libpower],
)
