/*
 * <p>Title: config.h</p>
 * <p>Description: </p>
 * <p>Copyright: Copyright (c) 2019 FDU NiSL </p>
 *
 * @author Bob Huang
 * @date Nov 23, 2019 
 * @version 1.0
 */

#ifndef CONFIG_H_
#define CONFIG_H_

#ifdef __cplusplus
extern "C" {
#endif

#define NTM_SHMRING_NAME "/ntm-shm-ring"

// #define NTS_CONFIG_FILE "/etc/nts.cfg"
#define NTS_CONFIG_FILE "./nts.cfg"

// check whether the connect ip addr is vaild or not, if vaild return 0 else return -1
int ip_is_vaild(char * addr);

/* load configuration from specified configuration file name */
int load_conf(const char *fname);

/* print setted configuration */
void print_conf();

/* free the calloced memory */
void free_conf();


#ifdef __cplusplus
};
#endif

#endif /* CONFIG_H_ */
