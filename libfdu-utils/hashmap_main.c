#include <stdio.h>
#include <stdlib.h>

#include "hash_map.h"

int main() {

    HashMap fd_table;
    fd_table = createHashMap(NULL, NULL);

    int sockfd = 42;
    Put(fd_table, &sockfd, NULL);

    int tmpfd = 42;
    if (Exists(fd_table, &tmpfd)) {
        printf("sockfd %d exists. \n", tmpfd);
    } else {
        printf("sockfd %d doesn't exists. \n", tmpfd);
    }
    
    Clear(fd_table);
    fd_table = NULL;

    return 0;
}
