AM_DEFAULT_SOURCE_EXT = .cpp
AM_CPPFLAGS = -I$(top_srcdir)

bin_PROGRAMS = \
	pseq-monitor

pseq_monitor_SOURCES = \
	argument.cpp \
	main.cpp \
	pgood_monitor.cpp \
	runtime_monitor.cpp \
	ucd90160.cpp \
	ucd90160_defs.cpp

pseq_monitor_LDADD = \
	$(top_builddir)/libpower.la \
	$(PHOSPHOR_LOGGING_LIBS) \
	${PHOSPHOR_DBUS_INTERFACES_LIBS} \
	$(SDBUSPLUS_LIBS) \
	$(SDEVENTPLUS_LIBS)

pseq_monitor_CXXFLAGS = \
	$(PHOSPHOR_LOGGING_CFLAGS) \
	${PHOSPHOR_DBUS_INTERFACES_CFLAGS} \
	$(SDBUSPLUS_CFLAGS) \
	$(SDEVENTPLUS_CFLAGS)
