bin_PROGRAMS = ntb-proxy


INC_DIR = include
UTILS_DIR = utils

ntb_proxy_SOURCES = main.c nt_log.h		\
				config.c $(INC_DIR)/config.h 			\
				ntb_hw_intel.c $(INC_DIR)/ntb_hw_intel.h 	\
				ntb.c $(INC_DIR)/ntb.h 						\
				ntb_mem.c $(INC_DIR)/ntb_mem.h 			    \
				ntb_proxy.c $(INC_DIR)/ntb_proxy.h 			\
				ntlink_parser.c $(INC_DIR)/ntlink_parser.h  \
				ntm_msg.c $(INC_DIR)/ntm_msg.h 				\
				ntm_ntp_shm.c $(INC_DIR)/ntm_ntp_shm.h 			\
				ntm_ntp_shmring.c $(INC_DIR)/ntm_ntp_shmring.h 	\
				ntp_ntm_shm.c $(INC_DIR)/ntp_ntm_shm.h 			\
				ntp_ntm_shmring.c $(INC_DIR)/ntp_ntm_shmring.h 	\
				ntp_nts_msg.c $(INC_DIR)/ntp_nts_msg.h 				\
				ntp_nts_shm.c $(INC_DIR)/ntp_nts_shm.h 				\
				ntp_nts_shmring.c $(INC_DIR)/ntp_nts_shmring.h 		\
				gcc_builtins.h nt_atomic.h		\
				$(UTILS_DIR)/hashmap.c  	$(UTILS_DIR)/include/hashmap.h \
				$(UTILS_DIR)/array.c  		$(UTILS_DIR)/include/array.h	\
				$(UTILS_DIR)/utils.c  	$(UTILS_DIR)/include/utils.h 		\	
				$(UTILS_DIR)/include/data_struct.h

# Compiler options. Here we are adding the include directory
# to be searched for headers included in the source code.
ntb_proxy_CPPFLAGS = -I$(top_srcdir)/include -I$(top_srcdir)/src/include -I$(top_srcdir)/src/utils/include	\
						-I$(RTE_SDK)/build/lib -I$(RTE_SDK)/build/include -I$(RTE_SDK)/build/include/generic \
						-D_BSD_SOURCE -DHAVE_GCC_C11_ATOMICS

ntb_proxy_LDADD = -lz -lrt -lpthread -ldl
