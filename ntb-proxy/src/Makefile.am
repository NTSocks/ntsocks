bin_PROGRAMS = ntb-proxy


INC_DIR = include


ntb_proxy_SOURCES = main.c func.c nt_log.h ntp_msg.h		\
				ntb-proxy.c include/ntb-proxy.h 		
			

# Compiler options. Here we are adding the include directory
# to be searched for headers included in the source code.
ntb_proxy_CPPFLAGS = -I$(top_srcdir)/include -I$(top_srcdir)/src/include -D_BSD_SOURCE -DHAVE_GCC_C11_ATOMICS


ntb_proxy_LDADD = -lz -lrt -lpthread -ldl