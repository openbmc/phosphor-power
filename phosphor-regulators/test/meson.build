phosphor_regulators_tests_include_directories = include_directories(
    '.',
    'actions'
)

phosphor_regulators_tests_source_files = [
    'device_tests.cpp',
    'id_map_tests.cpp',
    'rail_tests.cpp',
    'rule_tests.cpp',

    'actions/action_environment_tests.cpp',
    'actions/action_utils_tests.cpp',
    'actions/and_action_tests.cpp',
    'actions/not_action_tests.cpp',
    'actions/or_action_tests.cpp',
    'actions/run_rule_action_tests.cpp',
    'actions/set_device_action_tests.cpp'
]

test('phosphor-regulators-tests',
     executable('phosphor-regulators-tests',
                phosphor_regulators_tests_source_files,
                dependencies: [
                    gmock,
                    gtest,
                ],
                link_with: phosphor_regulators_library,
                implicit_include_directories: false,
                include_directories: [
                    phosphor_regulators_include_directories,
                    phosphor_regulators_tests_include_directories
                ]
     )
)
