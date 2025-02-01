test(
    'psutils-tests',
    executable(
        'psutils-tests',
        'test_updater.cpp',
        'test_utils.cpp',
        'test_version.cpp',
        '../updater.cpp',
        '../aei_updater.cpp',
        '../utils.cpp',
        '../version.cpp',
        dependencies: [gtest, gmock, nlohmann_json_dep, phosphor_logging],
        implicit_include_directories: false,
        include_directories: [libpower_inc, libi2c_inc, libi2c_dev_mock_inc],
        link_args: dynamic_linker,
        build_rpath: get_option('oe-sdk').allowed() ? rpath : '',
        link_with: [libpower, libi2c_dev_mock],
    ),
)
