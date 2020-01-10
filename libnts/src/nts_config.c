/*
 * <p>Title: nts_config.c</p>
 * <p>Description: </p>
 * <p>Copyright: Copyright (c) 2019 </p>
 *
 * @author Bob Huang
 * @date Nov 23, 2019 
 * @version 1.0
 */


#include "nts.h"
#include "nts_config.h"
#include "nt_log.h"

#define MAX_BUF_LEN 1024
#define MAX_KEY_LEN 64
#define MAX_VAL_LEN 256
DEBUG_SET_LEVEL(DEBUG_LEVEL_DEBUG);


struct nts_config NTS_CONFIG = {
        /* set default configuration */
        .tcp_timewait = 0,
        .tcp_timeout = 1000,
        .local_has_ntbdev = 1
};

nts_context_t nts_ctx = NULL;


static int trim(char s[])
{
    int n;
    for (n = strlen(s) - 1; n >= 0; n--) {
        if (s[n] != ' ' && s[n] != '\t' && s[n] != '\n')
            break;
        s[n + 1] = '\0';
    }
    return n;
}

int load_conf(const char *fname)
{
    DEBUG("load ntb libnts configuration from conf file (%s)", fname);
    FILE *file = fopen(fname, "r");
    if (file == NULL) {
        printf("[Error]open %s failed.\n", fname);
        return -1;
    }


    char buf[MAX_BUF_LEN];
    int text_comment = 0;
    while (fgets(buf, MAX_BUF_LEN, file) != NULL) {
        trim(buf);
        // to skip text comment with flags /* ... */
        if (buf[0] != '#' && (buf[0] != '/' || buf[1] != '/')) {
            if (strstr(buf, "/*") != NULL) {
                text_comment = 1;
                continue;
            } else if (strstr(buf, "*/") != NULL) {
                text_comment = 0;
                continue;
            }
        }
        if (text_comment == 1) {
            continue;
        }


        int buf_len = strlen(buf);
        // ignore and skip the line with first chracter '#', '=' or '/'
        if (buf_len <= 1 || buf[0] == '#' || buf[0] == '=' || buf[0] == '/') {
            continue;
        }
        buf[buf_len - 1] = '\0';


        char _paramk[MAX_KEY_LEN] = {0}, _paramv[MAX_VAL_LEN] = {0};
        int _kv = 0, _klen = 0, _vlen = 0;
        int i = 0;
        for (i = 0; i < buf_len; ++i) {
            if (buf[i] == ' ')
                continue;
            // scan param key name
            if (_kv == 0 && buf[i] != '=') {
                if (_klen >= MAX_KEY_LEN)
                    break;
                _paramk[_klen++] = buf[i];
                continue;
            } else if (buf[i] == '=') {
                _kv = 1;
                continue;
            }
            // scan param key value
            if (_vlen >= MAX_VAL_LEN || buf[i] == '#')
                break;
            _paramv[_vlen++] = buf[i];
        }
        if (strcmp(_paramk, "") == 0 || strcmp(_paramv, "") == 0)
            continue;
        // DEBUG("ntb monitor configuration %s=%s\n", _paramk, _paramv);
        if (strcmp(_paramk, "local_has_ntbdev") == 0) {
            NTS_CONFIG.local_has_ntbdev = atoi(_paramv);
        } else if (strcmp(_paramk, "local_nt_host") == 0) {
            NTS_CONFIG.local_nt_host = calloc(_vlen, sizeof(char));
            memcpy(NTS_CONFIG.local_nt_host, _paramv, _vlen);
        } else if(strcmp(_paramk, "nt_host")){
            NTS_CONFIG.nt_host = calloc(_vlen, sizeof(char));
            memcpy(NTS_CONFIG.nt_host, _paramv, _vlen);
        } else if(strcmp(_paramk, "tcp_timewait") == 0){
            NTS_CONFIG.tcp_timewait = atoi(_paramv);
        } else if(strcmp(_paramk, "tcp_timeout") == 0){
            NTS_CONFIG.tcp_timeout = atoi(_paramv);
        } else {
            fclose(file);
            return 1;
        }
    }
    fclose(file);
    return 0;
}


void print_conf()
{
    printf("nts configuration: tcp_timewait=%d, tcp_timeout=%d, local_has_ntbdev=%d, local_nt_host=%s, nt_host=%s\n",NTS_CONFIG.tcp_timewait,NTS_CONFIG.tcp_timeout, NTS_CONFIG.local_has_ntbdev, NTS_CONFIG.local_nt_host, NTS_CONFIG.nt_host);
}
