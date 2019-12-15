
#ifndef NTM_MONITOR_H
#define NTM_MONITOR_H


#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <pthread.h>

#include "nt_log.h"
#include "ntm_shm.h"
#include "nts_shm.h"

#ifdef __cplusplus
extern "C" {
#endif



struct ntm_nts_context {
	
	/**
	 *	for ntm shm receive queue
	 */
	ntm_shm_context_t shm_recv_ctx;
	pthread_t shm_recv_thr;
	int shm_recv_signal;

	/**
	 * for ntm shm send queue
	 */
	nts_shm_context_t shm_send_ctx;
	pthread_t shm_send_thr;
	int shm_send_signal;


} ntm_nts_context;

typedef struct ntm_nts_context* ntm_nts_context_t;


struct ntm_ntp_context {

};

typedef struct ntm_ntp_context* ntm_ntp_context_t;

struct ntm_manager {
	ntm_nts_context_t nts_ctx;
	ntm_ntp_context_t ntp_ctx;
} ntm_manager;

typedef struct ntm_manager* ntm_manager_t;




int print_monitor();


#ifdef __cplusplus
};
#endif

#endif // end NTM_MONITOR_H 
