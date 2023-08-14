sequencer_src = []

if sequencer == 'ucd90160'
    ucd90160_defs_cpp = custom_target(
        'ucd90160_defs.cpp',
        command: [
            prog_python, '@INPUT0@',
            '-i', '@INPUT1@', '-o', meson.current_build_dir(),
        ],
        input: [
            'gen-ucd90160-defs.py',
            get_option('ucd90160-yaml'),
            'templates/ucd90160_defs.mako.cpp'
        ],
        output: 'ucd90160_defs.cpp',
    )

    sequencer_src += ['ucd90160.cpp', ucd90160_defs_cpp]

elif sequencer == 'mihawk-cpld'
    sequencer_src += 'mihawk-cpld.cpp'
endif

executable(
    'pseq-monitor',
    error_hpp,
    'main.cpp',
    'pgood_monitor.cpp',
    'runtime_monitor.cpp',
    sequencer_src,
    dependencies: [
        libi2c_dep,
        nlohmann_json_dep,
        phosphor_dbus_interfaces,
        phosphor_logging,
        sdbusplus,
        sdeventplus,
    ],
    include_directories: '..',
    install: true,
    link_with: libpower,
)
