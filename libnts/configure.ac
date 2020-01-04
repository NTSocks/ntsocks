dnl Process this file with autoconf to produce a configure script.

AC_PREREQ(2.59)
AC_INIT(libnts, 1.0.0, bobohuang.sun@qq.com)
AC_CONFIG_AUX_DIR(config)
AC_CONFIG_HEADER(config.h)

AC_CANONICAL_SYSTEM

AM_INIT_AUTOMAKE([-Wall])

m4_ifdef([AM_PROG_AR], [AM_PROG_AR])

dnl Checks for programs
AC_PROG_CC
AC_PROG_CXX
AC_PROG_LN_S

dnl Check if Libtool is present
dnl Libtool is used for building share libraries
AC_ENABLE_STATIC
AC_ENABLE_SHARED
AC_PROG_LIBTOOL

dnl Initialize Libtool
LT_INIT

# Check for C++11
AC_LANG_PUSH([C++])
AX_CXX_COMPILE_STDCXX_11([noext])
AC_LANG_POP([C++])

# Configure pthreads.
ACX_PTHREAD([have_pthread=yes])


# Define gtest variables
GTEST_VERSION="1.7.0"
if test "x$have_pthread" = "xyes"; then
  GTEST_CPPFLAGS="-DGTEST_HAS_PTHREAD=1"
  GTEST_CXXFLAGS="$PTHREAD_CFLAGS"
  GTEST_LDFLAGS=
  GTEST_LIBS="$PTHREAD_LIBS"
else
  GTEST_CPPFLAGS="-DGTEST_HAS_PTHREAD=1"
  GTEST_CXXFLAGS=
  GTEST_LDFLAGS=
  GTEST_LIBS=
fi
#AC_SUBST输出一个变量到由configure生成的文件中
AC_SUBST([GTEST_VERSION])
AC_SUBST([GTEST_CPPFLAGS])
AC_SUBST([GTEST_CXXFLAGS])
AC_SUBST([GTEST_LDFLAGS])
AC_SUBST([GTEST_LIBS])

dnl Checks for libraries


dnl Checks for header files


dnl Checks for typedefs, structures, and compiler characteristics


dnl Checks for library functions


AC_CONFIG_FILES(Makefile
                lib/Makefile
				lib/gtest/Makefile
                include/Makefile
                src/Makefile
                tests/Makefile
                examples/Makefile)
AC_OUTPUT

AC_MSG_RESULT([
libnts $VERSION is now configured
Configure Information:
C Compiler        : $CC
    DEFS            :   $DEFS
    CPPFLAGS        :   $CPPFLAGS
    CFLAGS          :   $CFLAGS

  C++ Compiler      : $CXX
    DEFS            :   $DEFS
    CPPFLAGS        :   $CPPFLAGS
    CXXFLAGS        :   $CXXFLAGS

  Linker            : $LD
    LDFLAGS         :   $LDFLAGS
    LIBS            :   $LIBS

  Google Test 
    GTEST_CPPFLAGS  : $GTEST_CPPFLAGS
    GTEST_CXXFLAGS  : $GTEST_CXXFLAGS
    GTEST_LDFLAGS   : $GTEST_LDFLAGS
    GTEST_LIBS      : $GTEST_LIBS
])