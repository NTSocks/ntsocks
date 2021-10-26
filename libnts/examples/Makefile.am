#######################################
# The list of executables we are building seperated by spaces
# the 'bin_' indicates that these build products will be installed
# in the $(bindir) directory. For example /usr/bin
#bin_PROGRAMS=hello-ntsock

# Because a.out is only a sample program we don't want it to be installed.
# The 'noinst_' prefix indicates that the following targets are not to be
# installed.
# noinst_PROGRAMS= hello-nts hello-ntsock-client hello-ntsock-server
noinst_PROGRAMS= hello-ntsock client-ntsock partition_server partition_client


#######################################
# Build information for each executable. The variable name is derived
# by use the name of the executable with each non alpha-numeric character is
# replaced by '_'. So a.out becomes a_out and the appropriate suffex added.
# '_SOURCES' for example.

# Sources for the a.out 
hello_ntsock_SOURCES = hello-nts.c
client_ntsock_SOURCES = hello-nts-client.c
partition_server_SOURCES = partition_server.c 
partition_client_SOURCES = partition_client.c
# hello_ntsock_client_SOURCES= hello-ntsock-client.c
# hello_ntsock_server_SOURCES= hello-ntsock-server.c

# Linker options for a.out
#hello_ntsock_LDFLAGS = $(top_builddir)/src/libnts.la

# 通过_LDADD告诉Automake需要链接哪些库
# hello_ntsock_LDADD = $(top_builddir)/src/lib/libnts.la -lrt
# client_ntsock_LDADD = $(top_builddir)/src/lib/libnts.la -lrt
# hello_ntsock_server_LDADD = $(top_builddir)/src/lib/libnts.la -lrt 
# hello_ntsock_client_LDADD = $(top_builddir)/src/lib/libnts.la -lrt 

# Compiler options for a.out
hello_ntsock_CPPFLAGS = -I$(top_srcdir)/include
client_ntsock_CPPFLAGS = -I$(top_srcdir)/include
partition_server_CPPFLAGS = -I$(top_srcdir)/include
partition_client_CPPFLAGS = -I$(top_srcdir)/include
# hello_ntsock_server_CPPFLAGS = -I$(top_srcdir)/include
# hello_ntsock_client_CPPFLAGS = -I$(top_srcdir)/include
