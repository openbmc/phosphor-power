project(
    'phosphor-power',
    'cpp',
    default_options: [
        'warning_level=3',
        'werror=true',
        'cpp_std=c++23',
        'buildtype=debugoptimized',
        'prefix=/usr'
    ],
    license: 'Apache-2.0',
    version: '1.0',
    meson_version: '>=1.1.1',
)

cxx = meson.get_compiler('cpp')

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


gmock = dependency('gmock', disabler: true, required: false)
gtest = dependency('gtest', main: true, disabler: true, required: false)
if not gtest.found() or not gmock.found() and build_tests.enabled()
  gtest_opts = import('cmake').subproject_options()
  gtest_opts.add_cmake_defines({
  'BUILD_SHARED_LIBS': 'ON'
  })
  gtest_proj = import('cmake').subproject('googletest',
                      options: gtest_opts,
                      required: false)
  if gtest_proj.found()
    gtest = declare_dependency(
      dependencies: [
        dependency('threads'),
        gtest_proj.dependency('gtest'),
        gtest_proj.dependency('gtest_main'),
      ]
    )
    gmock = gtest_proj.dependency('gmock')
  else
    assert(false, 'Googletest is required if tests are enabled')
  endif
endif



phosphor_dbus_interfaces = dependency('phosphor-dbus-interfaces')
phosphor_logging = dependency('phosphor-logging')
prog_python = import('python').find_installation('python3')
sdbusplus = dependency('sdbusplus')
sdbuspp = find_program('sdbus++')
sdeventplus = dependency('sdeventplus')
pthread = dependency('threads')
stdplus = dependency('stdplus')
boost = dependency('boost')
fmt = dependency('fmt')
libgpiodcxx = dependency('libgpiodcxx',
    default_options: ['bindings=cxx'])

if cxx.has_header('nlohmann/json.hpp')
    nlohmann_json_dep = declare_dependency()
else
    subproject('nlohmann', required: false)
    nlohmann_json_dep = declare_dependency(
        include_directories: [
            'subprojects/nlohmann-json/single_include',
            'subprojects/nlohmann-json/single_include/nlohmann',
        ]
    )
endif

if cxx.has_header('CLI/CLI.hpp')
    cli11_dep = declare_dependency()
else
    cli11_dep = dependency('CLI11')
endif

systemd = dependency('systemd')
servicedir = systemd.get_variable('systemdsystemunitdir')

services = [
    ['supply-monitor', 'power-supply-monitor@.service'],
    ['sequencer-monitor', 'pseq-monitor-pgood.service'],
    ['sequencer-monitor', 'pseq-monitor.service'],
    ['supply-monitor-ng', 'phosphor-psu-monitor.service'],
    ['regulators', 'phosphor-regulators.service'],
    ['regulators', 'phosphor-regulators-config.service'],
    ['regulators', 'phosphor-regulators-monitor-enable.service'],
    ['regulators', 'phosphor-regulators-monitor-disable.service'],
    ['power-control', 'phosphor-power-control.service'],
]

foreach service : services
    if get_option(service[0])
        configure_file(input: 'services/' + service[1],
                 output: service[1],
                 copy: true,
                 install_dir: servicedir)
    endif
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
    'INPUT_HISTORY_SYNC_GPIO', get_option('input-history-sync-gpio'))
conf.set_quoted(
    'PSU_JSON_PATH', '/usr/share/phosphor-power/psu.json')
conf.set(
    'SEQUENCER', sequencer_class)
conf.set10(
    'DEVICE_ACCESS', get_option('device-access'))
conf.set10('IBM_VPD', get_option('ibm-vpd'))

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
        nlohmann_json_dep,
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

if get_option('regulators')
    subdir('phosphor-regulators')
endif
if get_option('sequencer-monitor')
    subdir('power-sequencer')
endif
if get_option('power-control')
    subdir('phosphor-power-sequencer')
endif
if get_option('supply-monitor')
    subdir('power-supply')
endif
if get_option('supply-monitor-ng')
    subdir('phosphor-power-supply')
endif
if get_option('utils')
    subdir('tools/power-utils')
endif
if get_option('tests').enabled()
    subdir('test')
endif
if get_option('cold-redundancy')
    subdir('cold-redundancy')
endif
