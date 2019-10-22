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
        include_directories: '../../..',
        link_with: [
            libpower,
        ],
        objects: record_manager,
    )
)
