test(
    'nvtest',
    executable(
        'nvtest', 'nvtest.cpp',
        dependencies: [
            gmock,
            gtest,
        ],
        link_args: dynamic_linker,
        build_rpath: get_option('oe-sdk').allowed() ? rpath : '',
        implicit_include_directories: false,
        include_directories: '..',
    )
)

test(
    'file_descriptor_tests',
    executable(
        'file_descriptor_tests', 'file_descriptor_tests.cpp',
        dependencies: [
            gtest,
        ],
        link_args: dynamic_linker,
        build_rpath: get_option('oe-sdk').allowed() ? rpath : '',
        implicit_include_directories: false,
        include_directories: '..',
    )
)
