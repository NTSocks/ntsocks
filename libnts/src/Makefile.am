#######################################
# The list of libraries we are building seperated by spaces.
# The 'lib_' indicates that these build products will be installed
# in the $(libdir) directory. For example /usr/lib
lib_LTLIBRARIES = lib/libnts.la

#######################################
# Build information for each library

# Sources for libTest
lib_libnts_la_SOURCES = socket.c

# Linker options libTestProgram
lib_libnts_la_LDFLAGS =
#libnts_la_LDFLAGS = -module -avoid-version

# Compiler options. Here we are adding the include directory
# to be searched for headers included in the source code.
lib_libnts_la_CPPFLAGS = -I$(top_srcdir)/include
#libnts_la_CCFLAGS = -I$(top_srcdir)/include

# Specify the Library depencies via `_LIBADD`
lib_libnts_la_LIBADD = -lz