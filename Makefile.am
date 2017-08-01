AM_DEFAULT_SOURCE_EXT = .cpp

noinst_LTLIBRARIES = libpower.la
libpower_la_LDFLAGS = -static
libpower_la_LIBADD = \
	$(PHOSPHOR_LOGGING_LIBS) \
	$(SDBUSPLUS_LIBS)
libpower_la_CXXFLAGS = \
	$(PHOSPHOR_LOGGING_CFLAGS) \
	$(SDBUSPLUS_CFLAGS)

libpower_la_SOURCES = \
	pmbus.cpp \
	timer.cpp \
	utility.cpp

SUBDIRS = .
