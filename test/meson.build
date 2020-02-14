test(
    'nvtest',
    executable(
        'nvtest', 'nvtest.cpp',
        dependencies: [
            gmock,
            gtest,
        ],
        link_args: dynamic_linker,
        build_rpath: get_option('oe-sdk').enabled() ? rpath : '',
        implicit_include_directories: false,
        include_directories: '..',
    )
)
