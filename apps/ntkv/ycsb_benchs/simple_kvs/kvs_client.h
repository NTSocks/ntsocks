#ifndef YCSB_C_KVS_CLIENT_H_
#define YCSB_C_KVS_CLIENT_H_

#include <iostream>
#include <stdio.h>
#include <string>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>

extern int error_num;

namespace ycsbc {

struct message{
    uint8_t cmd;
    uint16_t key_len;
    uint16_t value_len;
    uint8_t data[0];
}__attribute__ ((packed));
typedef struct message message_t;
#define MESSAGE_SIZE sizeof(struct message)

struct reply{
    uint8_t err_no;
    uint16_t key_len;
    uint16_t value_len;
    uint8_t data[0];
}__attribute__ ((packed));
typedef struct reply reply_t;
#define REPLY_SIZE sizeof(struct reply)

enum CMD{
    CMD_PUT = 1,
    CMD_GET,
    CMD_BATCH_PUT,
    CMD_BATCH_GET,
    CMD_REMOVE,
    CMD_FREE,
    CMD_DESTROY,
    CMD_LEN
};

enum map_status{
    MAP_OK,
    MAP_REPLACE,
    MAP_MISSING,
    MAP_OMEM,
    MAP_ERR
};

typedef struct kvsContext {
    int err; /* Error flags, 0 when there is no error */
    char errstr[128]; /* String representation of error when applicable */
    int fd;
    int flags;

    char *obuf;
    int obuf_len;
} kvsContext;

typedef struct kvsReply {
    int type; /* REDIS_REPLY_* */
    long long integer; /* The integer when type is REDIS_REPLY_INTEGER */
    int len; /* Length of string */
    char *str; /* Used for both REDIS_REPLY_ERROR and REDIS_REPLY_STRING */
    size_t elements; /* number of elements, for REDIS_REPLY_ARRAY */
    struct kvsReply **element; /* elements vector for REDIS_REPLY_ARRAY */
} kvsReply;

class KVSClient {
public:
    // establish connection
    KVSClient(const char *host, int port){
        context_ = connect_kvs(host, port);
        if (!context_ || context_->err) {
            if (context_) {
                std::cerr << "Connect error: " << context_->errstr << std::endl;
                free(context_);
            } else {
                std::cerr << "Connect error: can't allocate redis context!" << std::endl;
            }
            exit(1);
        }
    }

    ~KVSClient(){
        free(context_);
    }

    int handle_msg(int cmd);

    // int Command(std::string cmd);

    kvsContext* connect_kvs(const char *host, int port);

    kvsContext* context() { return context_; }


private:
  void HandleError(kvsReply *reply, const char *hint);

  kvsContext *context_;
//   int slaves_;
};

}

#endif
