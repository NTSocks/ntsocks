AM_CPPFLAGS = -isystem $(top_srcdir)/lib/gtest/include -I$(top_srcdir)/include $(GTEST_CPPFLAGS)
AM_CXXFLAGS = $(GTEST_CXXFLAGS)
AM_LDFLAGS = $(GTEST_LDFLAGS) $(GTEST_LIBS) \
	     $(top_builddir)/lib/gtest/lib/libgtest_main.la \
	     $(top_builddir)/lib/gtest/lib/libgtest.la

# TESTS -- Programs run automatically by "make check"
# check_PROGRAMS -- Programs built by "make check" but not necessarily run
TESTS = 
check_PROGRAMS = 

TESTS += socket_test
check_PROGRAMS += socket_test
socket_test_SOURCES = socket_test.cpp
socket_test_LDADD = $(top_builddir)/src/lib_libnts_la-socket.o