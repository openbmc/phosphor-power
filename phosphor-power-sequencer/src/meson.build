phosphor_power_sequencer_include_directories = include_directories(
    '.',
    '../..'
)

phosphor_power_sequencer_library = static_library(
    'phosphor-power-sequencer',
    'config_file_parser.cpp',
    implicit_include_directories: false,
    dependencies: [
        nlohmann_json_dep
    ],
    include_directories: [
        phosphor_power_sequencer_include_directories
    ]
)

phosphor_power_sequencer = executable(
    'phosphor-power-control',
    'power_control_main.cpp',
    'power_control.cpp',
    'power_interface.cpp',
    'power_sequencer_monitor.cpp',
    'ucd90x_monitor.cpp',
    'ucd90160_monitor.cpp',
    'ucd90320_monitor.cpp',
    dependencies: [
        libgpiodcxx,
        nlohmann_json_dep,
        phosphor_logging,
        sdbusplus,
        sdeventplus,
        stdplus,
    ],
    link_with: [
        libpower
    ],
    implicit_include_directories: false,
    include_directories: phosphor_power_sequencer_include_directories,
    install: true
)
