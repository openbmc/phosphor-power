test(
    'common-code-tests',
    executable(
        'common-code-tests',
        'chassis_status_monitor_tests.cpp',
        'config_file_parser_error_tests.cpp',
        'file_descriptor_tests.cpp',
        'format_utils_tests.cpp',
        'json_parser_utils_tests.cpp',
        'nvtest.cpp',
        'temporary_file_tests.cpp',
        'temporary_subdirectory_tests.cpp',
        dependencies: [gmock, gtest],
        link_args: dynamic_linker,
        build_rpath: get_option('oe-sdk').allowed() ? rpath : '',
        link_with: [libpower],
        implicit_include_directories: false,
        include_directories: '..',
    ),
)
test(
    'chassis_log_test',
    executable(
        'chassis_log_test',
        dependencies: [
            gmock,
            gtest,
            libgpiodcxx,
            nlohmann_json_dep,
            phosphor_logging,
        ],
        implicit_include_directories: false,
        include_directories: '..',
        link_args: dynamic_linker,
        link_with: [libpower],
        build_rpath: get_option('oe-sdk').allowed() ? rpath : '',
    ),
)
