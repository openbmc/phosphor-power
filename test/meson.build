test(
    'common-code-tests',
    executable(
        'common-code-tests',
        'file_descriptor_tests.cpp',
        'nvtest.cpp',
        'temporary_file_tests.cpp',
        'temporary_subdirectory_tests.cpp',
        dependencies: [
            gmock,
            gtest,
        ],
        link_args: dynamic_linker,
        build_rpath: get_option('oe-sdk').allowed() ? rpath : '',
        link_with: [
            libpower
        ],
        implicit_include_directories: false,
        include_directories: '..',
    )
)
