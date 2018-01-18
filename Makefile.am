AM_DEFAULT_SOURCE_EXT = .cpp

noinst_LTLIBRARIES = libpower.la
libpower_la_LDFLAGS = -static
libpower_la_LIBADD = \
	-lstdc++fs \
	$(PHOSPHOR_LOGGING_LIBS) \
	$(SDBUSPLUS_LIBS) \
	$(PHOSPHOR_DBUS_INTERFACES_LIBS)
libpower_la_CXXFLAGS = \
	$(PHOSPHOR_LOGGING_CFLAGS) \
	$(SDBUSPLUS_CFLAGS) \
	$(PHOSPHOR_DBUS_INTERFACES_CFLAGS)

libpower_la_SOURCES = \
	gpio.cpp \
	pmbus.cpp \
	timer.cpp \
	utility.cpp \
	org/open_power/Witherspoon/Fault/error.cpp

nobase_nodist_include_HEADERS = \
	org/open_power/Witherspoon/Fault/error.hpp

BUILT_SOURCES = \
	org/open_power/Witherspoon/Fault/error.cpp \
	org/open_power/Witherspoon/Fault/error.hpp

org/open_power/Witherspoon/Fault/error.hpp: ${srcdir}/org/open_power/Witherspoon/Fault.errors.yaml
	@mkdir -p `dirname $@`
	$(SDBUSPLUSPLUS) -r $(srcdir) error exception-header org.open_power.Witherspoon.Fault > $@

org/open_power/Witherspoon/Fault/error.cpp: ${srcdir}/org/open_power/Witherspoon/Fault.errors.yaml
	@mkdir -p `dirname $@`
	$(SDBUSPLUSPLUS) -r $(srcdir) error exception-cpp org.open_power.Witherspoon.Fault > $@

SUBDIRS = . power-sequencer power-supply test power-supply/test
