phosphor_power_sequencer_include_directories = include_directories(
    '.',
    '../..'
)

phosphor_power_sequencer = executable(
    'phosphor-power-control',
    'power_control_main.cpp',
    'power_control.cpp',
    dependencies: [
        phosphor_logging,
        sdbusplus,
        sdeventplus,
        stdplus
    ],
    implicit_include_directories: false,
    include_directories: phosphor_power_sequencer_include_directories,
    install: true
)
