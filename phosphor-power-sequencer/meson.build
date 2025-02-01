install_subdir(
    'config_files',
    strip_directory: true,
    install_dir: get_option('datadir') / 'phosphor-power-sequencer',
)

subdir('src')

if get_option('tests').allowed()
    subdir('test')
endif
