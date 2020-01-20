/* SPDX-License-Identifier: BSD-3-Clause
 * Copyright(c) 2010-2014 Intel Corporation
 */
/*
 * This sample application is a simple NTB ping-pong test used for
 * latency measurement for remote write and local read. NTB configuration
 * is based on back-to-back type and shared memory window.
 */

#include <stdint.h>
#include <unistd.h>
#include <sys/types.h>
#include <inttypes.h>
#include <locale.h>


#include <rte_eal.h>
#include <rte_lcore.h>
#include <rte_pause.h>
#include <rte_rawdev.h>
#include <rte_ring.h>
#include <rte_mempool.h>
#include <rte_cycles.h>

#include <ntb.h>

#define NTB_BAR23 0x2
#define NTB_BAR45 0x4

#define LOOP_CNT 1000000
#define RUN_TIME 10

#define AVX512_BYTE   64

static uint64_t timer_period;

struct ntb_mw_conf {
	volatile uint64_t *mw_rflag23, *mw_lflag23;
	uint64_t *mw_rptr23, *mw_lptr23;
	uint64_t bw;
};

struct ntb_mw_conf mw_conf[RTE_MAX_LCORE];

static void
lcore_main(void)
{
	struct rte_rawdev_info info = {0};
	struct ntb_conf ntb_conf = {0};
	unsigned int i = 0;
	unsigned lcore_id;
	struct ntb_mw_conf *mconf;
	uint64_t prev_tsc, diff_tsc, cur_tsc, timer_tsc;
	uint64_t start_tsc, end_tsc;

	/* 8Byte data movement for latency test */
	uint64_t send_data = 1;
	uint64_t receive_data = 1;

	/* 64Byte using AVX512 to fill 4MB buffer */
	uint64_t send_data_avx0[8] = {0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17};
//	uint64_t send_data_avx1[8] = {0x90, 0x91, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97};

	uint64_t receive_data_avx0[8] = {0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0};
//	uint64_t receive_data_avx1[8] = {0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0};

	prev_tsc = 0;
	timer_tsc = 0;
	timer_period = RUN_TIME * rte_get_timer_hz();

	lcore_id = rte_lcore_id();
	printf("lcore index = %d, Hello from lcore = %d\n",rte_lcore_index(lcore_id), lcore_id);

	mconf = &mw_conf[rte_lcore_index(lcore_id)];

	*mconf->mw_rflag23 = 0;
	*mconf->mw_lflag23 = 0;
	mconf->bw = 0;

	info.dev_private = &ntb_conf;
	if (rte_rawdev_info_get(0, &info))
		rte_exit(EXIT_FAILURE, "rte_rawdev_info_get() failed\n");

//	printf("%s, rawdev count = %d\n", __func__, rte_rawdev_count());
//	printf("%s, NTB: %s num_queues %u\n",
//		__func__, info.driver_name, ntb_conf.num_queues);

	(ntb_conf.dev_type == 0) ? printf("Downstream\n"):printf("Upstream\n");

	if (rte_rawdev_configure(0, &info))
		rte_exit(EXIT_FAILURE, "rte_rawdev_configure() failed\n");

	if (ntb_conf.dev_type == 0) {
		*mconf->mw_rflag23 = 1;
		__asm__("mfence");

		start_tsc = rte_rdtsc_precise();

		for (i = 0; i < LOOP_CNT; i++) {

			send_data++;

			*mconf->mw_rflag23 = send_data;
			__asm__("mfence");
			send_data++;

			while (1) {
				if (*mconf->mw_lflag23 == send_data)
					break;
			}
		}
		end_tsc = rte_rdtsc_precise();

		printf("Round trip tests Lasted : %lg ns\n",
				(double)((end_tsc - start_tsc) / LOOP_CNT) /
					rte_get_tsc_hz() * NS_PER_S);
		printf("FT: Last data received %ld\n",send_data);
	}
	else {
		while (1) {
			if (*mconf->mw_lflag23 == 1)
				break;
		}

		for (i = 0; i < LOOP_CNT; i++) {

			receive_data++;
			while (1) {
				if (*mconf->mw_lflag23 == receive_data)
					break;
			}
			receive_data++;
			*mconf->mw_rflag23 = receive_data;
			__asm__("mfence");
		}
		printf("FT: Last data sent from US %ld\n",receive_data);
	}

	printf("NTB: BW test using single core starting.....\n");
	setlocale(LC_NUMERIC, "");

	*mconf->mw_rflag23 = 0;
	__asm__("mfence");

// Asynchronous write from DS; make sure to change BAR2 to WC
	if (ntb_conf.dev_type == 0) {

		mconf->mw_rptr23 = (uint64_t *)((uintptr_t)mconf->mw_rflag23);
		prev_tsc = rte_rdtsc();
		*mconf->mw_rflag23 = 1;
		__asm__("mfence");

		while(1) {
			for(i = 0; i < (NTB_MW_SZ/AVX512_BYTE); i++) {
				rte_mov64((uint8_t *)mconf->mw_rptr23,(uint8_t *)send_data_avx0);
				__asm__("mfence");
				mconf->mw_rptr23 += (sizeof(uint64_t));
			}

			mconf->mw_rptr23 = (uint64_t *)((uintptr_t)mconf->mw_rflag23);
			cur_tsc = rte_rdtsc();
			diff_tsc = cur_tsc - prev_tsc;
			if (timer_period > 0) {
				timer_tsc += diff_tsc;
				if (unlikely(timer_tsc >= timer_period)) {
					break;
				}
			}
			mconf->bw++;
			prev_tsc = cur_tsc;
		}

		*mconf->mw_rflag23 = 0;
		__asm__("mfence");

		printf("lcore is = %d 4MB; loop counter = %ld ; BW (bits/s) = %'ld \n",rte_lcore_id(),mconf->bw,(mconf->bw * NTB_MW_SZ * 8 )/RUN_TIME);
	}

	else {
		mconf->mw_lptr23 = (uint64_t *)((uintptr_t)mconf->mw_lflag23);

		while (1) {
			if (*mconf->mw_lflag23 == 1)
				break;
		}
/* who cares stale data I am reading for IOTG DB replication*/
		while (1) {
			for(i = 0; i < (NTB_MW_SZ/AVX512_BYTE); i++) {
				rte_mov64((uint8_t *)receive_data_avx0, (uint8_t *)mconf->mw_lptr23);
				mconf->mw_lptr23 += (sizeof(uint64_t));
			}
			mconf->mw_lptr23 = (uint64_t *)((uintptr_t)mconf->mw_lflag23);

			if (*mconf->mw_lflag23 == 0)
				break;
		}
	}

#if 0 // whiteboard with Edwin ; mw = 0 ; followed by mw = 0x1 and mw = 0x2 in busy loop

	printf("Writer writes values 0x1 and 0x2 in 4MB using AVX512\n");
	(void)timer_tsc;
	(void)diff_tsc;
	(void)cur_tsc;
	(void)prev_tsc;
	(void)send_data_avx;

	mconf->mw_rptr0 = (uint64_t *)((uintptr_t)mconf->mw_rflag);

	for (i = 0; i < NTB_MW_SZ/AVX_TRAN_PER_UNIT; i++) {
			rte_mov64((uint8_t *)mconf->mw_rptr0,0x0);
			mconf->mw_rptr0 += (sizeof(uint64_t));
	}

	while(1) {
		for (i = 0; i < NTB_MW_SZ/AVX_TRAN_PER_UNIT; i++) {
			rte_mov64((uint8_t *)mconf->mw_rptr0,(uint8_t *)send_data_avx0);
			mconf->mw_rptr0 += (sizeof(uint64_t));
		}
		mconf->mw_rptr0 = (uint64_t *)((uintptr_t)mconf->mw_rflag);

		for (i = 0; i < NTB_MW_SZ/AVX_TRAN_PER_UNIT; i++) {
			rte_mov64((uint8_t *)mconf->mw_rptr0,(uint8_t *)send_data_avx1);
			mconf->mw_rptr0 += (sizeof(uint64_t));
		}
		mconf->mw_rptr0 = (uint64_t *)((uintptr_t)mconf->mw_rflag);
	}

#endif

#if 0

	while (1) {

		for(i = 0; i < AVX_TRAN_PER_FIFO; i++) {
			rte_mov64((uint8_t *)mconf->mw_rptr0,(uint8_t *)send_data_avx);
			mconf->mw_rptr0 += (sizeof(uint64_t));
		}
		
		*mconf->mw_rflag = 1;
		mconf->mw_rptr0 = (uint64_t *)((uintptr_t)mconf->mw_rflag) + 1;

		while (1) {
			if (*mconf->mw_lflag == 0)
				break;
		}

		for(i = 0; i < AVX_TRAN_PER_FIFO; i++) {
			rte_mov64((uint8_t *)mconf->mw_rptr0,(uint8_t *)send_data_avx);
			mconf->mw_rptr0 += (sizeof(uint64_t));
		}

		*mconf->mw_rflag = 0;

		mconf->bw++;

		cur_tsc = rte_rdtsc();
		diff_tsc = cur_tsc - prev_tsc;
		if (timer_period > 0) {
			timer_tsc += diff_tsc;
			if (unlikely(timer_tsc >= timer_period)) {
				break;
			}
		}

		prev_tsc = cur_tsc;
		while (1) {
			if (*mconf->mw_lflag == 1)
				break;
		}

		mconf->mw_rptr0 = (uint64_t *)((uintptr_t)mconf->mw_rflag) + 1;
	}
	printf("new bw = %ld and BW (bits/s) = %'ld lcore is = %d\n",mconf->bw,(2 * mconf->bw * FIFO_ELEMENTS * 8)/BW_timer,rte_lcore_id());
#endif
}


static int
NTB_launch_one_lcore(__attribute__((unused)) void *dummy)
{
	lcore_main();
	return 0;
}

/*
 * The main function, which uses only one core
 */
int
main(int argc, char *argv[])
{
	unsigned nb_ports;
	unsigned lcore_id;
	unsigned int i;

	/* Initialize the Environment Abstraction Layer (EAL) and DFS. */
	int ret = rte_eal_init(argc, argv);
	if (ret < 0)
		rte_exit(EXIT_FAILURE, "Error with EAL initialization\n");

	nb_ports = rte_rawdev_count();
	if (nb_ports != 1)
		rte_exit(EXIT_FAILURE, "Error: number of "
				"rawdev port must be 1\n");

	for (i = 0; i < rte_lcore_count(); i++) {
		mw_conf[i].mw_rflag23 = rte_ntb_bar_create(0, NTB_BAR23);
		if ((mw_conf[i].mw_rflag23) == NULL)
			rte_exit(EXIT_FAILURE, "Problem getting remote\n");

		mw_conf[i].mw_lflag23 = rte_ntb_mw_create(0, NTB_BAR23, NTB_MW_SZ);
		if (mw_conf[i].mw_lflag23 == NULL)
			rte_exit(EXIT_FAILURE, "Problem getting local\n");

		mw_conf[i].mw_rptr23 = (uint64_t *)(uintptr_t)mw_conf[i].mw_rflag23 + 1;
		mw_conf[i].mw_lptr23 = (uint64_t *)(uintptr_t)mw_conf[i].mw_lflag23 + 1;
	}


	/* Call lcore_main on the master core only. */
	rte_eal_mp_remote_launch(NTB_launch_one_lcore, NULL, CALL_MASTER);
		RTE_LCORE_FOREACH_SLAVE(lcore_id) {
			if (rte_eal_wait_lcore(lcore_id) < 0) {
				ret = -1;
				break;
			}
		}

	return 0;
}
