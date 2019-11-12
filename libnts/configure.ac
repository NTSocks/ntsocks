dnl Process this file with autoconf to produce a configure script.

AC_PREREQ(2.59)
AC_INIT(libnts, 1.0.0, bobohuang.sun@qq.com)
AC_CONFIG_AUX_DIR(config)
AC_CONFIG_MACRO_DIR(config)
AC_CONFIG_HEADER(config.h)

AC_CANONICAL_SYSTEM

AM_INIT_AUTOMAKE([-Wall])

dnl Checks for programs
AC_PROG_CC
AC_PROG_CXX
AC_GNU_SOURCE
AC_PROG_LN_S

dnl Check if Libtool is present
dnl Libtool is used for building share libraries
AC_ENABLE_STATIC
AC_ENABLE_SHARED
AC_PROG_LIBTOOL

dnl Initialize Libtool
LT_INIT

dnl Checks for libraries


dnl Checks for header files


dnl Checks for typedefs, structures, and compiler characteristics


dnl Checks for library functions


AC_CONFIG_FILES(Makefile 
                include/Makefile
                src/Makefile
                examples/Makefile)
AC_OUTPUT