#ifndef CONFIG_H_
#define CONFIG_H_

#ifdef __cplusplus
extern "C"
{
#endif

#define NTM_LISTEN_IP "0.0.0.0"
#define NTM_LISTEN_PORT 9090

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
