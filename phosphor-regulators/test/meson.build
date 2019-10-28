test('phosphor-regulators-tests',
     executable('phosphor-regulators-tests',
                'device_tests.cpp',
                'id_map_tests.cpp',
                'rail_tests.cpp',
                'rule_tests.cpp',
                dependencies: [
                    gmock,
                    gtest,
                ],
                implicit_include_directories: false,
                include_directories: phosphor_regulators_includes,
     )
)

# subdir('actions')
