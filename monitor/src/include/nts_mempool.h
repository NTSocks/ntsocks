/*
 * <p>Title: nts_mempool.h</p>
 * <p>Description: </p>
 * <p>Copyright: Copyright (c) 2019 FDU NiSL</p>
 *
 * @author Bob Huang
 * @date Dec 5, 2019 
 * @version 1.0
 */

#ifndef NTS_MEMPOOL_H_
#define NTS_MEMPOOL_H_

#ifdef __cplusplus
extern "C" {
#endif


typedef struct nts_mempool {

} nts_mempool;
typedef struct nts_mempool *nts_mempool_t;

/**
 * create a memory pool with a chunk size and total size
 * return the pointer to the memory pool
 */
nts_mempool_t nts_mp_create(char *name, int chunk_size, size_t pool_size);

/**
 * allocate one chunk
 */
void *nts_mp_allocate_chunk(nts_mempool_t mp);

/**
 * free one chunk
 */
void nts_mp_free_chunk(nts_mempool_t mp, void *p);

/**
 * destroy the memory pool
 */
void nts_mp_destroy(nts_mempool_t mp);

/**
 * return the number of free chunks
 */
int nts_mp_get_free_chunks(nts_mempool_t mp);

#ifdef __cplusplus
};
#endif

#endif /* NTS_MEMPOOL_H_ */
