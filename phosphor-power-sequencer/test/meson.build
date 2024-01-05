test('phosphor-power-sequencer-tests',
     executable('phosphor-power-sequencer-tests',
                'rail_tests.cpp',
                dependencies: [
                    gtest,
                    nlohmann_json_dep
                ],
                link_args: dynamic_linker,
                build_rpath: get_option('oe-sdk').allowed() ? rpath : '',
                implicit_include_directories: false,
                include_directories: [
                    '.',
                    '../src'
                ]
     )
)
