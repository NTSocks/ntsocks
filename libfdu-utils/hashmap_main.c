#include "hash_map.h"

#include <stdio.h>
#include <string.h>

typedef uint32_t nt_sock_id;

struct nt_socket {
	nt_sock_id sockid;
	int socktype;
	uint32_t opts;

	uint32_t epoll;
	uint32_t events;
	uint64_t ep_data;
} nt_socket;
typedef struct nt_socket * nt_socket_t;

int main() {
    int id = 1;

    HashMap map = createHashMap(NULL, NULL);
    
    nt_socket_t sock = (nt_socket_t) calloc(1, sizeof(struct nt_socket));
    sock->sockid = 1;
    sock->socktype = 45;
    sock->epoll = 431;
    sock->ep_data = 43;
    Put(map, &sock->sockid, sock);

    nt_socket_t rs = NULL;
    rs = (nt_socket_t) Get(map, &id);
    if (rs == NULL) {
        printf("NULL is true\n");
    } else {
        printf("data: %d, %d \n", rs->sockid, rs->socktype);
    }

    free(rs);

    rs = (nt_socket_t) Get(map, &id);
    if (rs == NULL) {
        printf("NULL is true\n");
    } else {
        printf("data: %d, %d \n", rs->sockid, rs->socktype);
    }

    return 0;
}
