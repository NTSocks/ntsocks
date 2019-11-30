#######################################
# The list of libraries we are building seperated by spaces.
# The 'lib_' indicates that these build products will be installed
# in the $(libdir) directory. For example /usr/lib
lib_LTLIBRARIES = lib/libnts.la

include_HADERS = 

#######################################
# Build information for each library

INC_DIR = include

# Sources for libTest
lib_libnts_la_SOURCES = socket.c libnts.c \
		debug.c $(INC_DIR)/debug.h		\
		clock.c $(INC_DIR)/clock.h				\
		eventpoll.c $(INC_DIR)/eventpoll.h			\
		logger.c $(INC_DIR)/logger.h				\
		nts_ini_parser.c $(INC_DIR)/nts_ini_parser.h		\
		nts_memory.c $(INC_DIR)/nts_memory.h			\
		nts_msg.c $(INC_DIR)/nts_msg.h			\
		nts_shm.c $(INC_DIR)/nts_shm.h			\
		nts.c $(INC_DIR)/nts.h				\
		stat.c $(INC_DIR)/stat.h				\
		timer.c $(INC_DIR)/timer.h				

# Linker options libTestProgram
lib_libnts_la_LDFLAGS =
#libnts_la_LDFLAGS = -module -avoid-version

# Compiler options. Here we are adding the include directory
# to be searched for headers included in the source code.
lib_libnts_la_CPPFLAGS = -I$(top_srcdir)/include -I$(top_srcdir)/src/include
#libnts_la_CCFLAGS = -I$(top_srcdir)/include

# Specify the Library depencies via `_LIBADD`
lib_libnts_la_LIBADD = -lz