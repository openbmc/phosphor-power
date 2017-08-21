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
	pmbus.cpp \
	timer.cpp \
	utility.cpp \
	xyz/openbmc_project/Power/Fault/error.cpp

nobase_nodist_include_HEADERS = \
	xyz/openbmc_project/Power/Fault/error.hpp

BUILT_SOURCES = \
	xyz/openbmc_project/Power/Fault/error.cpp \
	xyz/openbmc_project/Power/Fault/error.hpp

xyz/openbmc_project/Power/Fault/error.hpp: ${srcdir}/xyz/openbmc_project/Power/Fault.errors.yaml
	@mkdir -p `dirname $@`
	$(SDBUSPLUSPLUS) -r $(srcdir) error exception-header xyz.openbmc_project.Power.Fault > $@

xyz/openbmc_project/Power/Fault/error.cpp: ${srcdir}/xyz/openbmc_project/Power/Fault.errors.yaml
	@mkdir -p `dirname $@`
	$(SDBUSPLUSPLUS) -r $(srcdir) error exception-cpp xyz.openbmc_project.Power.Fault > $@

SUBDIRS = . power-sequencer
