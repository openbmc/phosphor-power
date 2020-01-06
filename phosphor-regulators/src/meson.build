phosphor_regulators_include_directories = include_directories(
    '.',
    '../..',
    'actions'
)

phosphor_regulators_library_source_files = [
    'id_map.cpp',

    'actions/if_action.cpp'
]

phosphor_regulators_library = static_library(
    'phosphor-regulators',
    phosphor_regulators_library_source_files,
    implicit_include_directories: false,
    include_directories: phosphor_regulators_include_directories
)

phosphor_regulators = executable(
    'phosphor-regulators',
    'main.cpp',
    gen_intf_cpp,
    gen_intf_hpp,
    dependencies: [
        sdbusplus,
        sdeventplus,
        libi2c_dep
    ],
    link_with: phosphor_regulators_library,
    implicit_include_directories: false,
    include_directories: phosphor_regulators_include_directories,
    install: true
)
