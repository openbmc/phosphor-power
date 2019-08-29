AM_DEFAULT_SOURCE_EXT = .cpp
AM_CPPFLAGS = -I$(top_srcdir)

bin_PROGRAMS = \
	psutils

psutils_SOURCES = \
	argument.cpp \
	version.cpp \
	main.cpp

psutils_LDADD = \
	$(top_builddir)/libpower.la \
	$(PHOSPHOR_LOGGING_LIBS)

psutils_CXXFLAGS = \
	$(PHOSPHOR_LOGGING_CFLAGS)
