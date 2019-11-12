#######################################
# The list of executables we are building seperated by spaces
# the 'bin_' indicates that these build products will be installed
# in the $(bindir) directory. For example /usr/bin
#bin_PROGRAMS=hello-ntsock

# Because a.out is only a sample program we don't want it to be installed.
# The 'noinst_' prefix indicates that the following targets are not to be
# installed.
noinst_PROGRAMS=hello-ntsock


#######################################
# Build information for each executable. The variable name is derived
# by use the name of the executable with each non alpha-numeric character is
# replaced by '_'. So a.out becomes a_out and the appropriate suffex added.
# '_SOURCES' for example.

# Sources for the a.out 
hello_ntsock_SOURCES= hello-ntsock.c

# Linker options for a.out
#hello_ntsock_LDFLAGS = $(top_builddir)/src/libnts.la

# 通过_LDADD告诉Automake需要链接哪些库
#hello_ntsock_LDADD = ../src/libnts.la
hello_ntsock_LDADD = $(top_builddir)/src/libnts.la

# Compiler options for a.out
hello_ntsock_CPPFLAGS = -I$(top_srcdir)/include