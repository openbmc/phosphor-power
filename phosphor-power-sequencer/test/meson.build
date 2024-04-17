test('phosphor-power-sequencer-tests',
     executable('phosphor-power-sequencer-tests',
                'config_file_parser_error_tests.cpp',
                'config_file_parser_tests.cpp',
                'format_utils_tests.cpp',
                'rail_tests.cpp',
                'standard_device_tests.cpp',
                dependencies: [
                    gmock,
                    gtest,
                    nlohmann_json_dep
                ],
                link_args: dynamic_linker,
                build_rpath: get_option('oe-sdk').allowed() ? rpath : '',
                link_with: [
                    phosphor_power_sequencer_library,
                    libpower
                ],
                implicit_include_directories: false,
                include_directories: [
                    '.',
                    '../src',
                    '../..'
                ]
     )
)
