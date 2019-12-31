/*
 * <p>Title: ntb-proxy.h</p>
 * <p>Description: </p>
 * <p>Copyright: Copyright (c) 2019 </p>
 *
 * @author Bob Huang
 * @date Nov 12, 2019 
 * @version 1.0
 */

#ifndef NTB_PROXY_H_
#define NTB_PROXY_H_

/*----------------------------------------------------------------------------*/
struct ntp_config {

	/* config for the connection between local and remote nt-monitor  */
	int remote_ntp_tcp_timewait;
	int remote_ntp_tcp_timeout;

};

extern int print_hello();

extern int add(int a, int b);

#endif /* NTB_PROXY_H_ */
