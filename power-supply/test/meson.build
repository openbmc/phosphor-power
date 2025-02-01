test(
    'test_records',
    executable(
        'test_records',
        'test_records.cpp',
        dependencies: [
            gtest,
            phosphor_dbus_interfaces,
            phosphor_logging,
            sdbusplus,
        ],
        implicit_include_directories: false,
        include_directories: '../..',
        link_args: dynamic_linker,
        build_rpath: get_option('oe-sdk').allowed() ? rpath : '',
        link_with: [libpower],
        objects: record_manager,
    ),
)
