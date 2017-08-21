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

# Create configured output
AC_CONFIG_FILES([Makefile power-sequencer/Makefile])
AC_OUTPUT
