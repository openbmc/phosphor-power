phosphor_regulators_include_directories = include_directories(
    '.',
    'actions'
)

phosphor_regulators_source_files = [
    'id_map.cpp'
]

phosphor_regulators_library = static_library(
    'phosphor-regulators',
    phosphor_regulators_source_files,
    implicit_include_directories: false,
    include_directories: phosphor_regulators_include_directories
)
