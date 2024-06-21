phosphor_power_sequencer_include_directories = include_directories(
    '.',
    '../..'
)

phosphor_power_sequencer_library = static_library(
    'phosphor-power-sequencer',
    'config_file_parser.cpp',
    'device_finder.cpp',
    'pmbus_driver_device.cpp',
    'rail.cpp',
    'services.cpp',
    'standard_device.cpp',
    'ucd90160_device.cpp',
    'ucd90320_device.cpp',
    'ucd90x_device.cpp',
    implicit_include_directories: false,
    dependencies: [
        nlohmann_json_dep,
        phosphor_dbus_interfaces,
        phosphor_logging,
        sdbusplus,
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
    dependencies: [
        libgpiodcxx,
        nlohmann_json_dep,
        phosphor_logging,
        sdbusplus,
        sdeventplus,
        stdplus,
    ],
    link_with: [
        phosphor_power_sequencer_library,
        libpower
    ],
    implicit_include_directories: false,
    include_directories: phosphor_power_sequencer_include_directories,
    install: true
)
