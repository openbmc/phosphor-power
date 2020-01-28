test('phosphor-power-supply-tests',
     executable('phosphor-power-supply-tests',
                'power_supply_tests.cpp',
                'mock.cpp',
                dependencies: [
                    gmock,
                    gtest,
                    sdbusplus,
                    sdeventplus,
                    phosphor_logging,
                ],
                implicit_include_directories: false,
                include_directories: [
                    '.',
                    '..',
                    '../..'
                ],
                link_args: dynamic_linker,
                build_rpath: get_option('oe-sdk').enabled() ? rpath : '',
                objects: power_supply,
     )
)
