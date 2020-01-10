#######################################
# The list of libraries we are building seperated by spaces.
# The 'lib_' indicates that these build products will be installed
# in the $(libdir) directory. For example /usr/lib
lib_LTLIBRARIES = lib/libnts.la

include_HADERS = 

#######################################
# Build information for each library

INC_DIR = include
UTILS_DIR = utils

# Sources for libTest
lib_libnts_la_SOURCES = socket.c libnts.c 		        \
		nts_config.c 									\
		nts_api.c $(INC_DIR)/nts_api.h					\
		nts.c $(INC_DIR)/nts.h 							\
		debug.c $(INC_DIR)/debug.h						\
		clock.c $(INC_DIR)/clock.h						\
		eventpoll.c $(INC_DIR)/eventpoll.h				\
		logger.c $(INC_DIR)/logger.h					\
		nts_errno.c $(INC_DIR)/nts_errno.h				\
		nts_event.c $(INC_DIR)/nts_event.h				\
		$(INC_DIR)/nt_atomic.h 							\
		$(INC_DIR)/gcc_builtins.h						\
		$(INC_DIR)/nt_log.h 							\
		nts_epoll.c $(INC_DIR)/nts_epoll.h				\
		nts_ini_parser.c $(INC_DIR)/nts_ini_parser.h	\
		nts_memory.c $(INC_DIR)/nts_memory.h			\
		nts_msg.c $(INC_DIR)/nts_msg.h					\
		stat.c $(INC_DIR)/stat.h						\
		timer.c $(INC_DIR)/timer.h						\
		nts_shmring.c $(INC_DIR)/nts_shmring.h			\
		nts_shm.c $(INC_DIR)/nts_shm.h					\
		ntm_shmring.c $(INC_DIR)/ntm_shmring.h			\
		ntm_shm.c $(INC_DIR)/ntm_shm.h					\
		$(UTILS_DIR)/array.c $(UTILS_DIR)/include/array.h 								\
		$(UTILS_DIR)/hash_map.c $(UTILS_DIR)/include/hash_map.h							\
		$(UTILS_DIR)/utils.c $(UTILS_DIR)/include/utils.h 								\
		$(UTILS_DIR)/nt_spsc_shm_queue.c $(UTILS_DIR)/include/nt_spsc_shm_queue.h 		\
		$(UTILS_DIR)/include/data_struct.h $(UTILS_DIR)/include/nt_atomic.h 			\
		$(UTILS_DIR)/include/gcc_builtins.h 
						

# Linker options libTestProgram
lib_libnts_la_LDFLAGS =
#libnts_la_LDFLAGS = -module -avoid-version

# Compiler options. Here we are adding the include directory
# to be searched for headers included in the source code.
lib_libnts_la_CPPFLAGS = -I$(top_srcdir)/include -I$(top_srcdir)/src/include 			\
						 -I$(top_srcdir)/src/utils/include -D_BSD_SOURCE -DHAVE_GCC_C11_ATOMICS
						 
#lib_libnts_la_CFLAGS = -I$(top_srcdir)/include -I$(top_srcdir)/src/include 
					   
#libnts_la_CCFLAGS = -I$(top_srcdir)/include

# Specify the Library depencies via `_LIBADD`
lib_libnts_la_LIBADD = -lz -lrt -lpthread -ldl -luuid
