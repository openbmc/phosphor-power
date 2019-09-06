project(
    'phosphor-power',
    'cpp',
    default_options: [
        'warning_level=3',
        'werror=true',
        'cpp_std=c++17',
        'prefix=/usr'
    ],
    license: 'Apache-2.0',
    version: '1.0',
)

build_tests = get_option('tests')

if get_option('oe-sdk').enabled()
  # Setup OE SYSROOT
  OECORE_TARGET_SYSROOT = run_command('sh', '-c', 'echo $OECORE_TARGET_SYSROOT').stdout().strip()
  if OECORE_TARGET_SYSROOT == ''
    error('Unable to get $OECORE_TARGET_SYSROOT, check your environment.')
  endif
  message('OE_SYSROOT: ' + OECORE_TARGET_SYSROOT)
  rpath = ':'.join([OECORE_TARGET_SYSROOT + '/lib', OECORE_TARGET_SYSROOT + '/usr/lib'])
  ld_so = run_command('sh', '-c', 'find ' + OECORE_TARGET_SYSROOT + '/lib/ld-*.so | sort -r -n | head -n1').stdout().strip()
  dynamic_linker = ['-Wl,-dynamic-linker,' + ld_so]
else
  dynamic_linker = []
endif


cppfs = meson.get_compiler('cpp').find_library('stdc++fs')
gmock = dependency('gmock', disabler: true, required: build_tests)
gtest = dependency('gtest', main: true, disabler: true, required: build_tests)
phosphor_dbus_interfaces = dependency('phosphor-dbus-interfaces')
phosphor_logging = dependency('phosphor-logging')
prog_python = import('python').find_installation('python')
sdbusplus = dependency('sdbusplus')
sdbuspp = find_program('sdbus++')
sdeventplus = dependency('sdeventplus')
pthread = dependency('threads')

systemd = dependency('systemd')
servicedir = systemd.get_pkgconfig_variable('systemdsystemunitdir')

services = [
    'power-supply-monitor@.service',
    'pseq-monitor-pgood.service',
    'pseq-monitor.service',
    'phosphor-psu-monitor.service',
    'phosphor-regulators.service'
]

foreach service : services
  configure_file(input: 'services/' + service,
                 output: service,
                 copy: true,
                 install_dir: servicedir)
endforeach

# Get the power sequencer class name
sequencer = get_option('power_sequencer')
if sequencer == 'ucd90160'
    sequencer_class = 'UCD90160'
elif sequencer == 'mihawk-cpld'
    sequencer_class = 'MihawkCPLD'
else
    # power sequencer is incorrect
    error('power sequencer is incorrect')
endif

conf = configuration_data()
conf.set_quoted(
    'INPUT_HISTORY_BUSNAME_ROOT', get_option('input-history-busname-root'))
conf.set_quoted(
    'INPUT_HISTORY_SENSOR_ROOT', get_option('input-history-sensor-root'))
conf.set_quoted(
    'PSU_JSON_PATH', '/usr/share/phosphor-power/psu.json')
conf.set(
    'SEQUENCER', sequencer_class)
conf.set10(
    'DEVICE_ACCESS', get_option('device-access'))

configure_file(output: 'config.h', configuration: conf)

# Ensure the generated header here winds up in the correct path in the build
# tree such that it actually get used and doesn't get found in the sysroot
# somewhere.  Meson doesn't allow path elements (rightfully so) when specifying
# the output filename of a target definition so the target must be defined in
# the directory where the artifacts need to be placed.  Do that now, because
# the generated source (cpp) is needed to define the library target.
subdir('org/open_power/Witherspoon/Fault')

libpower = static_library(
    'power',
    error_cpp,
    error_hpp,
    'gpio.cpp',
    'pmbus.cpp',
    'utility.cpp',
    dependencies: [
        cppfs,
        phosphor_dbus_interfaces,
        phosphor_logging,
        sdbusplus,
        sdeventplus,
    ],
)

libpower_inc = include_directories('.')

# Build the tools/i2c sub-directory first.  Other sub-directories depend on
# Meson variables defined there.
subdir('tools/i2c')

subdir('phosphor-regulators')
subdir('power-sequencer')
subdir('power-supply')
subdir('phosphor-power-supply')
subdir('tools/power-utils')
if get_option('tests').enabled()
    subdir('test')
endif
subdir('cold-redundancy')
