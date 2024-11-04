test(
    'test_version',
    executable(
        'test_version',
        'test_version.cpp',
        '../version.cpp',
        dependencies: [
            gtest,
            nlohmann_json_dep,
            phosphor_logging,
        ],
        implicit_include_directories: false,
        include_directories: libpower_inc,
        link_args: dynamic_linker,
        build_rpath: get_option('oe-sdk').allowed() ? rpath : '',
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
        '../version.cpp',
        dependencies: [
            gtest,
            gmock,
            nlohmann_json_dep,
            phosphor_logging,
        ],
        implicit_include_directories: false,
        include_directories: [
            libpower_inc,
            libi2c_inc,
            libi2c_dev_mock_inc
        ],
        link_args: dynamic_linker,
        build_rpath: get_option('oe-sdk').allowed() ? rpath : '',
        link_with: [
            libpower,
            libi2c_dev_mock
        ],
        objects: record_manager,
    )
)
