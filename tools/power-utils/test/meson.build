test(
    'test_version',
    executable(
        'test_version',
        'test_version.cpp',
        '../version.cpp',
        dependencies: [
            gtest,
            phosphor_logging,
        ],
        implicit_include_directories: false,
        include_directories: libpower_inc,
        link_args: dynamic_linker,
        build_rpath: get_option('oe-sdk').enabled() ? rpath : '',
        link_with: [
            libpower,
        ],
        objects: record_manager,
    )
)

test(
    'test_updater',
    executable(
        'test_updater',
        'test_updater.cpp',
        '../updater.cpp',
        dependencies: [
            gtest,
            gmock,
            phosphor_logging,
        ],
        implicit_include_directories: false,
        include_directories: [
            libpower_inc,
            libi2c_inc,
            libi2c_dev_mock_inc
        ],
        link_args: dynamic_linker,
        build_rpath: get_option('oe-sdk').enabled() ? rpath : '',
        link_with: [
            libpower,
            libi2c_dev_mock
        ],
        objects: record_manager,
    )
)
