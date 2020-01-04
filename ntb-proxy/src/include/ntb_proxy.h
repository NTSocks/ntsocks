/*
 * <p>Title: ntb_proxy.h</p>
 * <p>Description: </p>
 * <p>Copyright: Copyright (c) 2019 </p>
 *
 * @author Jing7
 * @date Nov 12, 2019 
 * @version 1.0
 */

#ifndef NTB_PROXY_H_
#define NTB_PROXY_H_

#include <rte_ring.h>

typedef struct rte_ring* ntp_rs_ring;

typedef struct ntp_ring_list_node
{
    ntp_rs_ring ring;
    ntp_ring_list_node* next_node;
}ntp_ring_list_node;

struct ntp_rs_ring ntp_ring_lookup(uint16_t src_port,uint16_t dst_port);



#endif /* NTB_PROXY_H_ */
