project(
    'phosphor-power',
    'cpp',
    default_options: [
        'warning_level=3',
        'werror=true',
        'cpp_std=c++17'
    ],
    license: 'Apache-2.0',
    version: '1.0',
)

build_tests = get_option('tests')

cppfs = meson.get_compiler('cpp').find_library('stdc++fs')
gmock = dependency('gmock', disabler: true, required: build_tests)
gtest = dependency('gtest', main: true, disabler: true, required: build_tests)
openpower_dbus_interfaces = dependency('openpower-dbus-interfaces')
phosphor_dbus_interfaces = dependency('phosphor-dbus-interfaces')
phosphor_logging = dependency('phosphor-logging')
prog_python = import('python').find_installation('python')
sdbusplus = dependency('sdbusplus')
sdbuspp = find_program('sdbus++')
sdeventplus = dependency('sdeventplus')

systemd = dependency('systemd')
servicedir = systemd.get_pkgconfig_variable('systemdsystemunitdir')

services = [
    'power-supply-monitor@.service',
    'pseq-monitor-pgood.service',
    'pseq-monitor.service'
]

foreach service : services
  configure_file(input: 'services/' + service,
                 output: service,
                 copy: true,
                 install_dir: servicedir)
endforeach

conf = configuration_data()
conf.set_quoted(
    'INPUT_HISTORY_BUSNAME_ROOT', get_option('input-history-busname-root'))
conf.set_quoted(
    'INPUT_HISTORY_SENSOR_ROOT', get_option('input-history-sensor-root'))
conf.set_quoted(
    'PSU_JSON_PATH', '/usr/share/phosphor-power/psu.json')
conf.set10(
    'UCD90160_DEVICE_ACCESS', get_option('ucd90160-access'))

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
        openpower_dbus_interfaces,
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
subdir('test')
