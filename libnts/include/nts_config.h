#ifndef CONFIG_H_
#define CONFIG_H_

#ifdef __cplusplus
extern "C"
{
#endif

#define NTM_SHMRING_NAME "/ntm-shm-ring"

// #define NTS_CONFIG_FILE "/etc/nts.cfg"
#define NTS_CONFIG_FILE "./nts.cfg"
#define NTS_MAX_CONCURRENCY 65536

    // check whether the connect ip addr is vaild or not, 
    //  if vaild return 0 else return -1
    int ip_is_vaild(char *addr);

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
