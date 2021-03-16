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
lib_libnts_la_SOURCES = socket.c libnts.c				\
		$(INC_DIR)/socket.h		        				\
		nts_config.c $(INC_DIR)/nt_errno.h				\
		nts_api.c $(INC_DIR)/nts_api.h					\
		nt_backlog.c $(INC_DIR)/nt_backlog.h 			\
		nts.c $(INC_DIR)/nts.h 							\
		nts_errno.c $(INC_DIR)/nts_errno.h				\
		nts_event.c $(INC_DIR)/nts_event.h				\
		nts_epoll.c $(INC_DIR)/nts_epoll.h				\
		nts_msg.c $(INC_DIR)/nts_msg.h					\
		nts_shmring.c $(INC_DIR)/nts_shmring.h			\
		nts_shm.c $(INC_DIR)/nts_shm.h					\
		ntm_shmring.c $(INC_DIR)/ntm_shmring.h			\
		ntm_shm.c $(INC_DIR)/ntm_shm.h					\
		nt_port.c $(INC_DIR)/nt_port.h					\
		$(UTILS_DIR)/nt_spsc_shm_queue.c 				\
		$(UTILS_DIR)/include/nt_spsc_shm_queue.h 		
						

# Linker options libTestProgram
lib_libnts_la_LDFLAGS =
#libnts_la_LDFLAGS = -module -avoid-version

# Compiler options. Here we are adding the include directory
# to be searched for headers included in the source code.
lib_libnts_la_CPPFLAGS = -I$(top_srcdir)/include 				\
						 -I$(top_srcdir)/src/include 			\
						 -I$(top_srcdir)/src/utils/include 		\
						 -I/usr/local/include 					\
						 -D_DEFAULT_SOURCE 						\
						 -DHAVE_GCC_C11_ATOMICS					\
						 -DNDEBUG
#-DENABLE_DEBUG

 
# Specify the Library depencies via `_LIBADD`
lib_libnts_la_LIBADD = -lz -lrt -lpthread -ldl -luuid -lfdu-utils
