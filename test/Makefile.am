AM_CPPFLAGS = -I$(top_srcdir)

# Run all 'check' test programs
TESTS = $(check_PROGRAMS)

check_PROGRAMS = nvtest
nvtest_CPPFLAGS = -Igtest $(GTEST_CPPFLAGS) $(AM_CPPFLAGS)

nvtest_CXXFLAGS = $(PTHREAD_CFLAGS)
nvtest_LDFLAGS = -lgtest_main -lgtest $(PTHREAD_LIBS) $(OESDK_TESTCASE_FLAGS)

nvtest_SOURCES = nvtest.cpp
