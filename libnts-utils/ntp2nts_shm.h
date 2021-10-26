#ifndef NTP2NTS_SHM_H_
#define NTP2NTS_SHM_H_

#include <stdio.h>

#include "shmring.h"
#include "ntp2nts_msg.h"
#include "shm_mempool.h"

#define RETRY_TIMES 1000
#define NTP_SHMADDR_SIZE 16

#ifdef __cplusplus
extern "C"
{
#endif

	typedef enum ntp_shm_stat
	{
		NTP_SHM_UNREADY = 0,
		NTP_SHM_READY,
		NTP_SHM_CLOSE,
		NTP_SHM_UNLINK
	} ntp_shm_stat;

	struct ntp_shm_context
	{
		ntp_shm_stat shm_stat;
		shmring_handle_t ntsring_handle;
		char shm_addr[NTP_SHMADDR_SIZE];
		size_t addrlen;

		size_t slot_size; // slot size for shm mempool
						  //	default slot_size = mtu_size in our case

		// for bulk operations on shmring
		char **node_idxs_cache;
		int *node_idx_value_cache;
		size_t *max_lens;

		// for shm mempool
		shm_mp_handler *mp_handler;
	};

	typedef struct ntp_shm_context *ntp_shm_context_t;

	/**
	 * used by libnts app or nt-monitor to create ntp_shm_context
	 * @param slot_size: the slot size of shm mempool. in our case, slot_size = mtu_size
	 */
	ntp_shm_context_t ntp_shm(size_t slot_size);

	/**
	 * used by nts shm server (consumer)
	 * 
	 */
	int ntp_shm_accept(
		ntp_shm_context_t shm_ctx, char *shm_addr, size_t addrlen);

	/**
	 * used by nt-monitor client (producer)
	 */
	int ntp_shm_connect(
		ntp_shm_context_t shm_ctx, char *shm_addr, size_t addrlen);

	/**
	 * used by nt-monitor to send message to libnts app
	 */
	int ntp_shm_send(
		ntp_shm_context_t shm_ctx, ntp_msg *buf);

	/**
	 * used by libnts app to receive message from nt-monitor
	 */
	int ntp_shm_recv(
		ntp_shm_context_t shm_ctx, ntp_msg **buf);

	//TODO: for bulk send/recv
	/**
	 * @brief  bulk send: used by nt-monitor to send bulk message to libnts app
	 * @note   
	 * @param  shm_ctx: 
	 * @param  **bulk: 
	 * @param  bulk_size: 
	 * @retval 
	 */
	int ntp_shm_send_bulk(
		ntp_shm_context_t shm_ctx, ntp_msg **bulk, size_t bulk_size);

	/**
	 * @brief  bulk recv: used by libnts app to receive message from nt-monitor
	 * @note   
	 * @param  shm_ctx: 
	 * @param  **bulk: 
	 * @param  bulk_size: 
	 * @retval the number of received ntp_msgs
	 */
	size_t ntp_shm_recv_bulk(
		ntp_shm_context_t shm_ctx,
		ntp_msg **bulk, size_t bulk_size);

	int ntp_shm_send_bulk_idx(
		ntp_shm_context_t shm_ctx, char **node_idxs,
		size_t *idx_lens, size_t bulk_size);

	size_t ntp_shm_recv_bulk_idx(
		ntp_shm_context_t shm_ctx, char **node_idxs,
		size_t *max_lens, size_t bulk_size);

	/**
	 * used by libnts to front the top or next-pop ntp_msg
	 */
	int ntp_shm_front(ntp_shm_context_t shm_ctx, ntp_msg **buf);

	/**
	 * @brief  move forward shmring read_index, and remove top element, no memory copy
	 * @note   
	 * @param  shm_ctx: 
	 * @retval if 0, success; else if -1, failed
	 */
	int ntp_shm_pop(ntp_shm_context_t shm_ctx);

	/**
	 * used by libnts app to close and unlink the shm ring buffer.
	 */
	int ntp_shm_close(ntp_shm_context_t shm_ctx);

	/**
	 * used by nt-monitor to disconnect the shm-ring based connection with libnts app
	 */
	int ntp_shm_nts_close(ntp_shm_context_t shm_ctx);

	/**
	 * used by libnts app or nt-monitor to free the memory of ntp_shm_context
	 */
	void ntp_shm_destroy(ntp_shm_context_t shm_ctx);

	/**
	 * used by libnts app and ntp to allocate ntpacket memory from shm mempool 
	 *	return 0 if success; return -1 if failed
	*/
	int ntp_shm_ntpacket_alloc(
		ntp_shm_context_t shm_ctx,
		ntp_msg **buf, size_t mtu_size);

	/**
	 * used by libnts and ntp to free ntpacket memory into shm mempool
	 */
	void ntp_shm_ntpacket_free(
		ntp_shm_context_t shm_ctx, ntp_msg **buf);

#ifdef __cplusplus
};
#endif

#endif /* NTP2NTS_SHM_H_ */
