AM_CPPFLAGS = -I$(top_srcdir)

# Run all 'check' test programs
TESTS = $(check_PROGRAMS)

check_PROGRAMS = test_records

test_records_CPPFLAGS = -Igtest $(GTEST_CPPFLAGS) $(AM_CPPFLAGS)

test_records_CXXFLAGS = $(PTHREAD_CFLAGS) \
	$(PHOSPHOR_DBUS_INTERFACES_CFLAGS) \
	$(PHOSPHOR_LOGGING_CFLAGS) \
	$(SDBUSPLUS_CFLAGS)

test_records_LDFLAGS = -lgtest_main -lgtest \
	$(PTHREAD_LIBS) $(OESDK_TESTCASE_FLAGS) \
	$(PHOSPHOR_DBUS_INTERFACES_LIBS) \
	$(PHOSPHOR_LOGGING_LIBS) \
	$(SDBUSPLUS_LIBS)

test_records_SOURCES = test_records.cpp

test_records_LDADD = ../record_manager.o

