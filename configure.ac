# Initialization
AC_PREREQ([2.69])
AC_INIT([witherspoon-pfault-analysis], [1.0],
[https://github.com/openbmc/witherspoon-pfault-analysis/issues])
AC_LANG([C++])
AC_CONFIG_HEADERS([config.h])
AM_INIT_AUTOMAKE([subdir-objects -Wall -Werror foreign dist-xz])
AM_SILENT_RULES([yes])

# Checks for programs.
AC_PROG_CXX
AM_PROG_AR
AC_PROG_INSTALL
AC_PROG_MAKE_SET

# Checks for typedefs, structures, and compiler characteristics.
AX_CXX_COMPILE_STDCXX_14([noext])
AX_APPEND_COMPILE_FLAGS([-Wall -Werror], [CXXFLAGS])

# Download gpio.h from github if necessary.
AC_CHECK_HEADER(linux/gpio.h,[HAVE_LINUX_GPIO_H=""],[HAVE_LINUX_GPIO_H="-I linux/gpio.h"])
AS_IF([test "$HAVE_LINUX_GPIO_H" != ""],
    AC_MSG_WARN([Could not find linux/gpio.h: Attempting to download locally for building from https://raw.githubusercontent.com/torvalds/linux/master/include/uapi/linux/gpio.h])
    AC_SUBST([BT_BMC_DL],[`mkdir -p linux;wget https://raw.githubusercontent.com/torvalds/linux/master/include/uapi/linux/gpio.h -O linux/gpio.h`])
)

PKG_CHECK_MODULES([SDBUSPLUS], [sdbusplus],,
    AC_MSG_ERROR(["Requires sdbusplus package."]))
PKG_CHECK_MODULES([PHOSPHOR_LOGGING], [phosphor-logging],,\
    AC_MSG_ERROR(["Requires phosphor-logging package."]))
PKG_CHECK_MODULES([PHOSPHOR_DBUS_INTERFACES], [phosphor-dbus-interfaces],,\
    AC_MSG_ERROR(["Requires phosphor-dbus-interfaces package."]))
# Check for sdbus++
AC_PATH_PROG([SDBUSPLUSPLUS], [sdbus++])
AS_IF([test "x$SDBUSPLUSPLUS" == "x"],
    AC_MSG_ERROR(["Requires sdbus++"]))

# Checks for library functions.
LT_INIT # Required for systemd linking

# Check/set gtest specific functions.
AX_PTHREAD([GTEST_CPPFLAGS="-DGTEST_HAS_PTHREAD=1"],[GTEST_CPPFLAGS="-DGTEST_HAS_PTHREAD=0"])
AC_SUBST(GTEST_CPPFLAGS)

# Test cases require SDK so only build if we're told to (and SDK is available)
AC_ARG_ENABLE([oe-sdk],
    AS_HELP_STRING([--enable-oe-sdk], [Link testcases absolutely against OE SDK so they can be ran within it.])
)
AC_ARG_VAR(OECORE_TARGET_SYSROOT,
    [Path to the OE SDK SYSROOT])
AS_IF([test "x$enable_oe_sdk" == "xyes"],
    AS_IF([test "x$OECORE_TARGET_SYSROOT" == "x"],
          AC_MSG_ERROR([OECORE_TARGET_SYSROOT must be set with --enable-oe-sdk])
    )
    AC_MSG_NOTICE([Enabling OE-SDK at $OECORE_TARGET_SYSROOT])
    [
        testcase_flags="-Wl,-rpath,\${OECORE_TARGET_SYSROOT}/lib"
        testcase_flags="${testcase_flags} -Wl,-rpath,\${OECORE_TARGET_SYSROOT}/usr/lib"
        testcase_flags="${testcase_flags} -Wl,-dynamic-linker,`find \${OECORE_TARGET_SYSROOT}/lib/ld-*.so | sort -r -n | head -n1`"
    ]
    AC_SUBST([OESDK_TESTCASE_FLAGS], [$testcase_flags])
)

AC_ARG_ENABLE([turn-off-ucd90160-access],
    AS_HELP_STRING([--enable-turn-off-ucd90160-access], [Turn off UCD90160 hardware access])
)

AC_ARG_VAR(UCD90160_DEVICE_ACCESS, [Turn off UCD90160 hardware access])

# Always define UCD90160_DEVICE_ACCESS unless explicitly turned off
AS_IF([test "x$enable_turn_off_ucd90160_access" != "xyes"],
      [UCD90160_DEVICE_ACCESS="yes"]
      AC_DEFINE_UNQUOTED([UCD90160_DEVICE_ACCESS], ["$UCD90160_DEVICE_ACCESS"], [Turn off UCD90160 hardware access])
)

# Create configured output
AC_CONFIG_FILES([Makefile power-sequencer/Makefile power-supply/Makefile test/Makefile power-supply/test/Makefile])
AC_OUTPUT
