/*
 * <p>Title: utils.h</p>
 * <p>Description: </p>
 * <p>Copyright: Copyright (c) 2019 FDU NiSL</p>
 *
 * @author Bob Huang
 * @date 12/23/19 
 * @version 1.0
 */

#include <arpa/inet.h>

#include "utils.h"

char * createUUID() {
    const char * c = "89ab";
    char * buf = (char *)malloc(37);
    char * p = buf;

    for (int n = 0; n < 16; ++n) {
        int b = rand() % 255;

        switch (n) {
            case 6: sprintf(p, "4%x", b % 15); break;
            case 8: sprintf(p, "%c%x", c[rand() % strlen(c)], b % 15); break;
            default: sprintf(p, "%02x", b); break;
        }

        p += 2;

        switch (n) {
            case 3:
            case 5:
            case 7:
            case 9:
                *p++ = '-'; break;
        }
    }
    *p = 0;
    return buf;
}

char * generate_uuid() {
    uuid_t uuid;
    char * str;

    str = (char *)calloc(UUID_LEN, sizeof(char));
    uuid_generate(uuid);
    uuid_unparse(uuid, str);

    return str;
}


int parse_sockaddr_port(struct sockaddr_in* saddr) {
    assert(saddr);

    int port;
    port = ntohs(saddr->sin_port);

    return port;
}


