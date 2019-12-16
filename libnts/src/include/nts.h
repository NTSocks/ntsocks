/*
 * <p>Title: nts.h</p>
 * <p>Description: </p>
 * <p>Copyright: Copyright (c) 2019 FDU NiSL </p>
 *
 * @author Bob Huang
 * @date Nov 23, 2019 
 * @version 1.0
 */

#ifndef NTS_H_
#define NTS_H_

#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>

#include "nts_shm.h"
#include "ntm_shm.h"

#ifdef __cplusplus
extern "C" {
#endif

struct nts_config {
	int tcp_timewait;
	int tcp_timeout;

};

struct nts_ntm_context {

	/**
	 * for nts shm receive queue
	 * receive from nt-monitor
	 */
	nts_shm_context_t shm_recv_ctx;
	pthread_t shm_recv_thr;
	int shm_recv_signal;

	/**
	 * for nts shm send queue
	 * send messages to libnts app
	 */
	ntm_shm_context_t shm_send_ctx;
	pthread_t shm_send_thr;
	int shm_send_signal;


};

typedef struct nts_ntm_context* nts_ntm_context_t;

struct nts_ntp_context {
	pthread_t shm_recv_thr;
	int shm_recv_signal;

	pthread_t shm_send_thr;
	int shm_send_signal;

};

typedef struct nts_ntp_context* nts_ntp_context_t;

struct nts_context {
	nts_ntm_context_t ntm_ctx;
	nts_ntp_context_t ntp_ctx;
} nts_context;

typedef struct nts_context* nts_context_t;

/*----------------------------------------------------------------------------*/
extern nts_context_t nts_ctx;
extern struct nts_config NTS_CONFIG;





#ifdef __cplusplus
};
#endif

#endif /* NTS_H_ */
