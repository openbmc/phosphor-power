test('phosphor-power-supply-tests',
     executable('phosphor-power-supply-tests',
                'power_supply_tests.cpp',
                'mock.cpp',
                dependencies: [
                    gmock,
                    gtest,
                    libgpiodcxx,
                    nlohmann_json_dep,
                    phosphor_logging,
                    sdbusplus,
                    sdeventplus,
                ],
                implicit_include_directories: false,
                include_directories: [
                    '.',
                    '..',
                    '../..'
                ],
                link_args: dynamic_linker,
                link_with: [
                  libpower,
                  ],
                build_rpath: get_option('oe-sdk').enabled() ? rpath : '',
                objects: power_supply,
     )
)
