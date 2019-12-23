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

#define NTS_CONFIG_FILE "../config/nts.cfg"

/* load configuration from specified configuration file name */
int load_conf(const char *fname);

/* print setted configuration */
void print_conf();



#ifdef __cplusplus
};
#endif

#endif /* CONFIG_H_ */
